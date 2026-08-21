#include "chat_log.h"

#include <stdio.h>
#include <string.h>

int jsonEscapeAppend(char *out, size_t cap, int o, const char *src, size_t n) {
  if (o < 0) return -1;
  for (size_t i = 0; i < n; ++i) {
    unsigned char c = (unsigned char)src[i];
    const char *rep = nullptr;
    char ubuf[8];
    switch (c) {
      case '"': rep = "\\\""; break;
      case '\\': rep = "\\\\"; break;
      case '\n': rep = "\\n"; break;
      case '\r': rep = "\\r"; break;
      case '\t': rep = "\\t"; break;
      default:
        if (c < 0x20) {
          snprintf(ubuf, sizeof(ubuf), "\\u%04x", c);
          rep = ubuf;
        }
        break;
    }
    size_t need = rep ? strlen(rep) : 1;
    if ((size_t)o + need >= cap) return -1;
    if (rep) {
      memcpy(out + o, rep, need);
      o += (int)need;
    } else {
      out[o++] = (char)c;
    }
  }
  return o;
}

static int rawAppend(char *out, size_t cap, int o, const char *s) {
  if (o < 0) return -1;
  size_t n = strlen(s);
  if ((size_t)o + n >= cap) return -1;
  memcpy(out + o, s, n);
  return o + (int)n;
}

void ChatLog::init(char *arena, size_t arenaCap, ChatTurn *turns,
                   size_t turnCap) {
  mArena = arena;
  mArenaCap = arenaCap;
  mTurns = turns;
  mTurnCap = turnCap;
  clear();
}

void ChatLog::clear() {
  mCount = 0;
  mUsed = 0;
}

void ChatLog::evictOldestPair() {
  size_t drop = mCount >= 2 ? 2 : mCount;
  if (drop == 0) return;
  size_t freed = 0;
  for (size_t i = 0; i < drop; ++i) freed += mTurns[i].len;
  memmove(mArena, mArena + freed, mUsed - freed);
  mUsed -= freed;
  for (size_t i = drop; i < mCount; ++i) {
    mTurns[i - drop] = mTurns[i];
    mTurns[i - drop].off -= freed;
  }
  mCount -= drop;
}

bool ChatLog::addTurn(uint8_t role, const char *text, size_t len) {
  if (len > mArenaCap / 2) len = mArenaCap / 2;  // clamp a pathological turn
  while (mCount >= mTurnCap || mUsed + len > mArenaCap) {
    if (mCount == 0) return false;
    evictOldestPair();
  }
  mTurns[mCount].role = role;
  mTurns[mCount].off = (uint32_t)mUsed;
  mTurns[mCount].len = (uint32_t)len;
  memcpy(mArena + mUsed, text, len);
  mUsed += len;
  ++mCount;
  return true;
}

const char *ChatLog::turnText(size_t i, size_t *len, uint8_t *role) const {
  if (i >= mCount) return nullptr;
  if (len) *len = mTurns[i].len;
  if (role) *role = mTurns[i].role;
  return mArena + mTurns[i].off;
}

int ChatLog::buildRequest(char *out, size_t cap, const char *model,
                          unsigned maxTokens, const char *systemPrompt,
                          const char *toolsJson) const {
  char head[256];
  snprintf(head, sizeof(head),
           "{\"model\":\"%s\",\"max_tokens\":%u,\"stream\":true,"
           "\"thinking\":{\"type\":\"disabled\"},"
           "\"output_config\":{\"effort\":\"low\"},\"system\":\"",
           model, maxTokens);
  int o = rawAppend(out, cap, 0, head);
  o = jsonEscapeAppend(out, cap, o, systemPrompt, strlen(systemPrompt));
  o = rawAppend(out, cap, o, "\",");
  if (toolsJson) {
    o = rawAppend(out, cap, o, "\"tools\":");
    o = rawAppend(out, cap, o, toolsJson);
    o = rawAppend(out, cap, o, ",");
  }
  o = rawAppend(out, cap, o, "\"messages\":[");
  for (size_t i = 0; i < mCount; ++i) {
    if (i) o = rawAppend(out, cap, o, ",");
    bool user = mTurns[i].role == CHAT_USER || mTurns[i].role == CHAT_USER_RAW;
    bool raw =
        mTurns[i].role == CHAT_USER_RAW || mTurns[i].role == CHAT_ASSISTANT_RAW;
    o = rawAppend(out, cap, o, "{\"role\":\"");
    o = rawAppend(out, cap, o, user ? "user" : "assistant");
    o = rawAppend(out, cap, o, "\",\"content\":");
    if (raw) {
      if (o >= 0 && (size_t)o + mTurns[i].len < cap) {
        memcpy(out + o, mArena + mTurns[i].off, mTurns[i].len);
        o += (int)mTurns[i].len;
      } else {
        o = -1;
      }
    } else {
      o = rawAppend(out, cap, o, "\"");
      o = jsonEscapeAppend(out, cap, o, mArena + mTurns[i].off, mTurns[i].len);
      o = rawAppend(out, cap, o, "\"");
    }
    o = rawAppend(out, cap, o, "}");
  }
  o = rawAppend(out, cap, o, "]}");
  if (o < 0 || (size_t)o >= cap) return -1;
  out[o] = 0;
  return o;
}
