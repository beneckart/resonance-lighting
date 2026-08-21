#include "sse_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int jsonUnescape(const char *src, size_t srcLen, char *out, size_t outCap,
                 size_t *consumed) {
  size_t o = 0;
  size_t i = 0;
  while (i < srcLen) {
    char c = src[i];
    if (c == '"') {
      if (consumed) *consumed = i + 1;
      return (int)o;
    }
    if (o + 4 >= outCap) return -1;  // room for a UTF-8 triple + NUL
    if (c != '\\') {
      out[o++] = c;
      ++i;
      continue;
    }
    if (i + 1 >= srcLen) return -1;
    char e = src[i + 1];
    i += 2;
    switch (e) {
      case '"': out[o++] = '"'; break;
      case '\\': out[o++] = '\\'; break;
      case '/': out[o++] = '/'; break;
      case 'b': out[o++] = '\b'; break;
      case 'f': out[o++] = '\f'; break;
      case 'n': out[o++] = '\n'; break;
      case 'r': out[o++] = '\r'; break;
      case 't': out[o++] = '\t'; break;
      case 'u': {
        if (i + 4 > srcLen) return -1;
        unsigned cp = 0;
        for (int k = 0; k < 4; ++k) {
          char h = src[i + k];
          cp <<= 4;
          if (h >= '0' && h <= '9') cp |= (unsigned)(h - '0');
          else if (h >= 'a' && h <= 'f') cp |= (unsigned)(h - 'a' + 10);
          else if (h >= 'A' && h <= 'F') cp |= (unsigned)(h - 'A' + 10);
          else return -1;
        }
        i += 4;
        if (cp >= 0xD800 && cp <= 0xDFFF) {
          out[o++] = '?';  // surrogate halves unsupported on-device
          if (cp <= 0xDBFF && i + 6 <= srcLen && src[i] == '\\' &&
              src[i + 1] == 'u')
            i += 6;  // swallow the low half too
        } else if (cp < 0x80) {
          out[o++] = (char)cp;
        } else if (cp < 0x800) {
          out[o++] = (char)(0xC0 | (cp >> 6));
          out[o++] = (char)(0x80 | (cp & 0x3F));
        } else {
          out[o++] = (char)(0xE0 | (cp >> 12));
          out[o++] = (char)(0x80 | ((cp >> 6) & 0x3F));
          out[o++] = (char)(0x80 | (cp & 0x3F));
        }
        break;
      }
      default: return -1;
    }
  }
  return -1;  // no closing quote seen
}

int jsonFindString(const char *json, size_t jsonLen, const char *key,
                   char *out, size_t outCap) {
  // Whitespace-tolerant: model-authored tool inputs arrive as
  // {"id": "9E5AF0"} with spaces, server SSE JSON is compact — accept both.
  char pat[48];
  int patLen = snprintf(pat, sizeof(pat), "\"%s\"", key);
  if (patLen <= 0 || (size_t)patLen >= sizeof(pat)) return -1;
  for (size_t i = 0; i + (size_t)patLen <= jsonLen; ++i) {
    if (memcmp(json + i, pat, (size_t)patLen) != 0) continue;
    size_t p = i + (size_t)patLen;
    while (p < jsonLen && (json[p] == ' ' || json[p] == '\t')) ++p;
    if (p >= jsonLen || json[p] != ':') continue;
    ++p;
    while (p < jsonLen && (json[p] == ' ' || json[p] == '\t')) ++p;
    if (p >= jsonLen || json[p] != '"') continue;  // not a string value
    ++p;
    return jsonUnescape(json + p, jsonLen - p, out, outCap, nullptr);
  }
  return -1;
}

long jsonFindInt(const char *json, size_t jsonLen, const char *key, long dflt) {
  char pat[48];
  int patLen = snprintf(pat, sizeof(pat), "\"%s\":", key);
  if (patLen <= 0 || (size_t)patLen >= sizeof(pat)) return dflt;
  for (size_t i = 0; i + (size_t)patLen <= jsonLen; ++i) {
    if (memcmp(json + i, pat, (size_t)patLen) != 0) continue;
    size_t p = i + (size_t)patLen;
    while (p < jsonLen && (json[p] == ' ' || json[p] == '\t')) ++p;
    if (p >= jsonLen) return dflt;
    if (json[p] == '-' || (json[p] >= '0' && json[p] <= '9'))
      return strtol(json + p, nullptr, 10);
    return dflt;  // key present but not a number
  }
  return dflt;
}

static bool contains(const char *hay, size_t n, const char *needle) {
  size_t m = strlen(needle);
  if (m > n) return false;
  for (size_t i = 0; i + m <= n; ++i)
    if (memcmp(hay + i, needle, m) == 0) return true;
  return false;
}

void SseParser::reset() {
  mLen = 0;
  mOverflow = false;
  mEvent[0] = 0;
}

void SseParser::feed(const uint8_t *bytes, size_t n, const SseCallbacks &cb) {
  for (size_t i = 0; i < n; ++i) {
    char c = (char)bytes[i];
    if (c == '\n') {
      if (!mOverflow) {
        mLine[mLen] = 0;
        handleLine(cb);
      }
      mLen = 0;
      mOverflow = false;
      continue;
    }
    if (c == '\r') continue;
    if (mLen + 1 >= kLineCap) {
      mOverflow = true;  // drop oversize line whole rather than split it
      continue;
    }
    mLine[mLen++] = c;
  }
}

void SseParser::handleLine(const SseCallbacks &cb) {
  if (mLen == 0) return;  // event boundary
  if (strncmp(mLine, "event: ", 7) == 0) {
    strncpy(mEvent, mLine + 7, sizeof(mEvent) - 1);
    mEvent[sizeof(mEvent) - 1] = 0;
    return;
  }
  if (strncmp(mLine, "data: ", 6) != 0) return;
  const char *json = mLine + 6;
  size_t jsonLen = mLen - 6;
  static char val[2048];

  if (strcmp(mEvent, "content_block_delta") == 0) {
    if (contains(json, jsonLen, "\"type\":\"text_delta\"")) {
      int n = jsonFindString(json, jsonLen, "text", val, sizeof(val));
      if (n >= 0 && cb.onText) cb.onText(cb.cx, val, (size_t)n);
    } else if (contains(json, jsonLen, "\"type\":\"input_json_delta\"")) {
      int n = jsonFindString(json, jsonLen, "partial_json", val, sizeof(val));
      if (n >= 0 && cb.onToolInput) cb.onToolInput(cb.cx, val, (size_t)n);
    }
  } else if (strcmp(mEvent, "content_block_start") == 0) {
    if (contains(json, jsonLen, "\"type\":\"tool_use\"")) {
      static char name[64], id[64];
      int nn = jsonFindString(json, jsonLen, "name", name, sizeof(name));
      int ni = jsonFindString(json, jsonLen, "id", id, sizeof(id));
      if (nn >= 0) name[nn] = 0;
      if (ni >= 0) id[ni] = 0;
      if (nn >= 0 && cb.onToolUseStart)
        cb.onToolUseStart(cb.cx, name, ni >= 0 ? id : "");
    }
  } else if (strcmp(mEvent, "content_block_stop") == 0) {
    if (cb.onBlockStop) cb.onBlockStop(cb.cx);
  } else if (strcmp(mEvent, "message_delta") == 0) {
    int n = jsonFindString(json, jsonLen, "stop_reason", val, sizeof(val));
    if (n >= 0 && cb.onStopReason) {
      val[n] = 0;
      cb.onStopReason(cb.cx, val);
    }
  } else if (strcmp(mEvent, "message_stop") == 0) {
    if (cb.onMessageStop) cb.onMessageStop(cb.cx);
  } else if (strcmp(mEvent, "error") == 0) {
    int n = jsonFindString(json, jsonLen, "message", val, sizeof(val));
    if (n >= 0) val[n] = 0;
    if (cb.onError) cb.onError(cb.cx, n >= 0 ? val : "(unparsed error)");
  }
}
