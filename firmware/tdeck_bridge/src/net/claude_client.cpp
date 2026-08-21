#include "claude_client.h"

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <string.h>

#include "../core/chat_log.h"
#include "../core/sse_parser.h"
#include "../store/store.h"
#include "agent_tools.h"
#include "anthropic_root_ca.h"
#include "net_mgr.h"
#include "tool_schema.h"

#define CHAT_ARENA_BYTES (48 * 1024)
#define CHAT_TURN_CAP 32
#define REQ_BUF_BYTES (20 * 1024)
#define ASSIST_BUF_BYTES (8 * 1024)
#define TURN_BUF_BYTES (10 * 1024)
#define DELTA_RING_BYTES 8192
#define STALL_TIMEOUT_MS 20000
#define CONNECT_TIMEOUT_MS 12000
#define MAX_ATTEMPTS 5
#define MAX_TOOLS 3
#define TOOL_INPUT_SLOT 1536
#define MAX_AGENT_ITERATIONS 8

static const char kSystemPrompt[] =
    "You are the operator console for a ~130-fixture solar lantern tree "
    "(\"Resonance\") at Burning Man, running on a pocket T-Deck with a 320x240 "
    "screen. Be terse and concrete. Fixtures are addressed by 6-hex short id "
    "(e.g. 9E5AB8) or 'all'; classes: downlight, perimeter, uplight, "
    "chandelier. Use the mesh tools; prefer mesh_census/node_status before "
    "commanding anything. Call one tool at a time. Fleet-wide actions need "
    "the operator's on-device confirmation and may be denied — report denials "
    "honestly. Firmware clamps all actuator limits regardless of requests. "
    "Output plain ASCII only: the display has no Unicode glyphs (no em dashes, "
    "curly quotes, or emoji).";

// ---- state shared between the net task (producer) and UI/CLI (consumer) ----
static ChatLog gLog;
static char *gReqBuf = nullptr;
static char *gAssistBuf = nullptr;
static size_t gAssistLen = 0;
static char *gTurnBuf = nullptr;
static char *gToolInputs = nullptr;  // MAX_TOOLS x TOOL_INPUT_SLOT
static char *gToolResult = nullptr;  // 4 KB
static char *gPending = nullptr;
static volatile bool gPendingSet = false;
static volatile ChatState gState = ChatState::IDLE;
static char gStatus[96] = "idle";
static volatile uint32_t gGeneration = 0;
static bool gMirror = true;

static char *gRing = nullptr;
static volatile uint32_t gRingHead = 0, gRingTail = 0;

static void ringPush(const char *s, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    uint32_t head = gRingHead;
    if (head - gRingTail >= DELTA_RING_BYTES) return;
    gRing[head % DELTA_RING_BYTES] = s[i];
    gRingHead = head + 1;
  }
}

size_t claudeReadDeltas(char *buf, size_t cap) {
  size_t n = 0;
  while (n < cap && gRingTail != gRingHead) {
    buf[n++] = gRing[gRingTail % DELTA_RING_BYTES];
    ++gRingTail;
  }
  return n;
}

ChatState claudeState() { return gState; }
const char *claudeStatusLine() { return gStatus; }
uint32_t claudeGeneration() { return gGeneration; }
void claudeMirrorSerial(bool on) { gMirror = on; }

static void setStatus(ChatState s, const char *msg) {
  gState = s;
  strlcpy(gStatus, msg, sizeof(gStatus));
}

bool claudeSubmit(const char *text) {
  if (gPendingSet || !gPending) return false;
  strlcpy(gPending, text, 1024);
  gPendingSet = true;
  setStatus(ChatState::QUEUED, "queued");
  return true;
}

bool claudeClearHistory() {
  if (gPendingSet) return false;
  gLog.clear();
  setStatus(ChatState::IDLE, "history cleared");
  return true;
}

// ------------------------------- SSE plumbing -------------------------------

struct StreamCx {
  bool done = false;
  bool sawError = false;
  char stopReason[32] = {0};
  char apiError[80] = {0};
  int toolCount = 0;
  bool inTool = false;
  bool toolOverflow = false;
  char toolName[MAX_TOOLS][48] = {};
  char toolId[MAX_TOOLS][64] = {};
  size_t toolLen[MAX_TOOLS] = {};
};
static StreamCx *gCx = nullptr;

static void onText(void *, const char *t, size_t n) {
  if (gAssistLen + n < ASSIST_BUF_BYTES - 1) {
    memcpy(gAssistBuf + gAssistLen, t, n);
    gAssistLen += n;
  }
  ringPush(t, n);
  if (gMirror) Serial.write((const uint8_t *)t, n);
  gState = ChatState::STREAMING;
}

static void onToolUseStart(void *cx, const char *name, const char *id) {
  StreamCx *s = (StreamCx *)cx;
  if (s->toolCount >= MAX_TOOLS) {
    s->toolOverflow = true;
    return;
  }
  s->inTool = true;
  strlcpy(s->toolName[s->toolCount], name, sizeof(s->toolName[0]));
  strlcpy(s->toolId[s->toolCount], id, sizeof(s->toolId[0]));
  s->toolLen[s->toolCount] = 0;
  char note[80];
  int n = snprintf(note, sizeof(note), "\n[tool: %s]", name);
  ringPush(note, (size_t)n);
  if (gMirror) Serial.printf("%s\n", note);
  setStatus(ChatState::STREAMING, name);
}

static void onToolInput(void *cx, const char *frag, size_t n) {
  StreamCx *s = (StreamCx *)cx;
  if (!s->inTool || s->toolCount >= MAX_TOOLS) return;
  char *slot = gToolInputs + s->toolCount * TOOL_INPUT_SLOT;
  size_t &len = s->toolLen[s->toolCount];
  if (len + n < TOOL_INPUT_SLOT - 1) {
    memcpy(slot + len, frag, n);
    len += n;
  }
}

static void onBlockStop(void *cx) {
  StreamCx *s = (StreamCx *)cx;
  if (s->inTool) {
    s->inTool = false;
    if (s->toolCount < MAX_TOOLS) ++s->toolCount;
  }
}

static void onStopReason(void *cx, const char *r) {
  strlcpy(((StreamCx *)cx)->stopReason, r, sizeof(StreamCx::stopReason));
}

static void onMessageStop(void *cx) { ((StreamCx *)cx)->done = true; }

static void onApiError(void *cx, const char *m) {
  StreamCx *s = (StreamCx *)cx;
  s->sawError = true;
  strlcpy(s->apiError, m, sizeof(StreamCx::apiError));
}

// -------------------------- HTTP + chunked decoding --------------------------

// 0 = clean end_turn, 1 = retryable failure, 2 = fatal, 3 = tool use pending.
static int runExchange(StreamCx *cx) {
  WiFiClientSecure tls;
  tls.setCACert(kAnthropicRootCa);
  tls.setTimeout(CONNECT_TIMEOUT_MS);
  setStatus(ChatState::CONNECTING, "thinking...");
  if (!tls.connect("api.anthropic.com", 443)) {
    setStatus(ChatState::BACKOFF, "tls connect failed");
    return 1;
  }

  int bodyLen = gLog.buildRequest(gReqBuf, REQ_BUF_BYTES, settings().model,
                                  1024, kSystemPrompt, kToolsJson);
  if (bodyLen < 0) {
    setStatus(ChatState::FAILED, "request too large");
    return 2;
  }
  char hdr[512];
  int hn = snprintf(hdr, sizeof(hdr),
                    "POST /v1/messages HTTP/1.1\r\n"
                    "Host: api.anthropic.com\r\n"
                    "x-api-key: %s\r\n"
                    "anthropic-version: 2023-06-01\r\n"
                    "content-type: application/json\r\n"
                    "content-length: %d\r\n"
                    "connection: close\r\n\r\n",
                    settings().apiKey, bodyLen);
  tls.write((const uint8_t *)hdr, hn);
  tls.write((const uint8_t *)gReqBuf, bodyLen);

  uint32_t lastByteMs = millis();
  int httpStatus = 0;
  bool chunked = false;
  char line[256];
  size_t ln = 0;
  bool headersDone = false;
  while (!headersDone) {
    if (!tls.connected() && !tls.available()) {
      setStatus(ChatState::BACKOFF, "closed in headers");
      return 1;
    }
    if (millis() - lastByteMs > STALL_TIMEOUT_MS) {
      setStatus(ChatState::BACKOFF, "header stall");
      return 1;
    }
    int c = tls.read();
    if (c < 0) {
      delay(5);
      continue;
    }
    lastByteMs = millis();
    if (c == '\r') continue;
    if (c != '\n') {
      if (ln < sizeof(line) - 1) line[ln++] = (char)c;
      continue;
    }
    line[ln] = 0;
    if (httpStatus == 0 && strncmp(line, "HTTP/1.", 7) == 0)
      httpStatus = atoi(line + 9);
    else if (strncasecmp(line, "transfer-encoding:", 18) == 0 &&
             strstr(line, "chunked"))
      chunked = true;
    else if (ln == 0)
      headersDone = true;
    ln = 0;
  }

  if (httpStatus != 200) {
    char body[256];
    size_t bn = 0;
    uint32_t t0 = millis();
    while (bn < sizeof(body) - 1 && millis() - t0 < 3000) {
      int c = tls.read();
      if (c < 0) {
        if (!tls.connected()) break;
        delay(5);
        continue;
      }
      body[bn++] = (char)c;
    }
    body[bn] = 0;
    char msg[80] = "";
    int mn = jsonFindString(body, bn, "message", msg, sizeof(msg));
    if (mn >= 0) msg[mn] = 0;
    char status[96];
    snprintf(status, sizeof(status), "HTTP %d %s", httpStatus,
             mn >= 0 ? msg : "");
    bool fatal = httpStatus == 400 || httpStatus == 401 || httpStatus == 403 ||
                 httpStatus == 404 || httpStatus == 413;
    setStatus(fatal ? ChatState::FAILED : ChatState::BACKOFF, status);
    Serial.printf("claude: %s\n", status);
    return fatal ? 2 : 1;
  }

  gAssistLen = 0;
  ++gGeneration;
  *cx = StreamCx();
  SseCallbacks cb = {};
  cb.cx = cx;
  cb.onText = onText;
  cb.onToolUseStart = onToolUseStart;
  cb.onToolInput = onToolInput;
  cb.onBlockStop = onBlockStop;
  cb.onStopReason = onStopReason;
  cb.onMessageStop = onMessageStop;
  cb.onError = onApiError;
  SseParser parser;
  parser.reset();

  long chunkRemaining = -1;
  char sizeLine[16];
  size_t sn = 0;
  uint8_t buf[512];
  while (!cx->done && !cx->sawError) {
    if (millis() - lastByteMs > STALL_TIMEOUT_MS) {
      setStatus(ChatState::BACKOFF, "stream stalled");
      return 1;
    }
    if (!tls.available()) {
      if (!tls.connected()) break;
      delay(5);
      continue;
    }
    if (!chunked) {
      int n = tls.read(buf, sizeof(buf));
      if (n > 0) {
        lastByteMs = millis();
        parser.feed(buf, (size_t)n, cb);
      }
      continue;
    }
    if (chunkRemaining < 0) {
      int c = tls.read();
      if (c < 0) continue;
      lastByteMs = millis();
      if (c == '\r') continue;
      if (c == '\n') {
        if (sn == 0) continue;
        sizeLine[sn] = 0;
        chunkRemaining = strtol(sizeLine, nullptr, 16);
        sn = 0;
        if (chunkRemaining == 0) break;
      } else if (sn < sizeof(sizeLine) - 1) {
        sizeLine[sn++] = (char)c;
      }
      continue;
    }
    size_t want = chunkRemaining > (long)sizeof(buf) ? sizeof(buf)
                                                     : (size_t)chunkRemaining;
    int n = tls.read(buf, want);
    if (n > 0) {
      lastByteMs = millis();
      parser.feed(buf, (size_t)n, cb);
      chunkRemaining -= n;
      if (chunkRemaining == 0) chunkRemaining = -1;
    }
  }
  tls.stop();

  if (cx->sawError) {
    char status[96];
    snprintf(status, sizeof(status), "api: %s", cx->apiError);
    setStatus(ChatState::BACKOFF, status);
    return 1;
  }
  gAssistBuf[gAssistLen] = 0;

  if (strcmp(cx->stopReason, "tool_use") == 0 && cx->toolCount > 0)
    return 3;

  if (gAssistLen == 0) {
    setStatus(ChatState::BACKOFF, "empty response");
    return 1;
  }
  gLog.addTurn(CHAT_ASSISTANT, gAssistBuf, gAssistLen);
  if (gMirror) Serial.println();
  char status[64];
  snprintf(status, sizeof(status), "ok (%s)",
           cx->stopReason[0] ? cx->stopReason : "end");
  setStatus(ChatState::IDLE, status);
  return 0;
}

// ------------------------- agent tool-turn assembly -------------------------

static int rawAppend(char *out, size_t cap, int o, const char *s) {
  if (o < 0) return -1;
  size_t n = strlen(s);
  if ((size_t)o + n >= cap) return -1;
  memcpy(out + o, s, n);
  return o + (int)n;
}

// Record the assistant's text + tool_use blocks, run each tool, and append
// the tool_result turn. Returns false on assembly overflow (fatal).
static bool handleToolTurn(StreamCx *cx) {
  int o = rawAppend(gTurnBuf, TURN_BUF_BYTES, 0, "[");
  if (gAssistLen > 0) {
    o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, "{\"type\":\"text\",\"text\":\"");
    o = jsonEscapeAppend(gTurnBuf, TURN_BUF_BYTES, o, gAssistBuf, gAssistLen);
    o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, "\"},");
  }
  for (int i = 0; i < cx->toolCount; ++i) {
    if (i) o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, ",");
    o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, "{\"type\":\"tool_use\",\"id\":\"");
    o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, cx->toolId[i]);
    o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, "\",\"name\":\"");
    o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, cx->toolName[i]);
    o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, "\",\"input\":");
    const char *slot = gToolInputs + i * TOOL_INPUT_SLOT;
    if (cx->toolLen[i] > 0) {
      ((char *)slot)[cx->toolLen[i]] = 0;
      o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, slot);
    } else {
      o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, "{}");
    }
    o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, "}");
  }
  o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, "]");
  if (o < 0) return false;
  gLog.addTurn(CHAT_ASSISTANT_RAW, gTurnBuf, (size_t)o);

  // Execute and build the tool_result turn.
  o = rawAppend(gTurnBuf, TURN_BUF_BYTES, 0, "[");
  for (int i = 0; i < cx->toolCount; ++i) {
    const char *slot = gToolInputs + i * TOOL_INPUT_SLOT;
    bool ok = agentExecuteTool(cx->toolName[i], slot, cx->toolLen[i],
                               gToolResult, 4096);
    Serial.printf("tool %s -> %s%.120s\n", cx->toolName[i], ok ? "" : "ERR: ",
                  gToolResult);
    if (i) o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, ",");
    o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o,
                  "{\"type\":\"tool_result\",\"tool_use_id\":\"");
    o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, cx->toolId[i]);
    o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, "\",\"content\":\"");
    o = jsonEscapeAppend(gTurnBuf, TURN_BUF_BYTES, o, gToolResult,
                         strlen(gToolResult));
    o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, "\"");
    if (!ok) o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, ",\"is_error\":true");
    o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, "}");
  }
  if (cx->toolOverflow && o >= 0) {
    // Shouldn't happen with the one-tool prompt, but never leave a tool_use
    // without a result: unhandled extras were never captured, so say so.
    Serial.println("tool overflow: >MAX_TOOLS calls in one turn");
  }
  o = rawAppend(gTurnBuf, TURN_BUF_BYTES, o, "]");
  if (o < 0) return false;
  gLog.addTurn(CHAT_USER_RAW, gTurnBuf, (size_t)o);
  return true;
}

// ---------------------------------- task ------------------------------------

static void netTask(void *) {
  static StreamCx cx;
  for (;;) {
    if (!gPendingSet) {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }
    if (netState() != NetState::ONLINE || !netSntpSynced()) {
      setStatus(ChatState::QUEUED, netState() == NetState::ONLINE
                                       ? "queued (clock unsynced)"
                                       : "queued (link down)");
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }
    if (!storeHasApiKey()) {
      setStatus(ChatState::FAILED, "no api key (set key ...)");
      gPendingSet = false;
      continue;
    }

    gLog.addTurn(CHAT_USER, gPending, strlen(gPending));
    int iterations = 0;
    int attempt = 0;
    for (;;) {
      int rc = runExchange(&cx);
      if (rc == 3) {  // tool use
        if (++iterations >= MAX_AGENT_ITERATIONS) {
          setStatus(ChatState::FAILED, "agent iteration cap");
          break;
        }
        if (!handleToolTurn(&cx)) {
          setStatus(ChatState::FAILED, "tool turn overflow");
          break;
        }
        attempt = 0;
        continue;
      }
      if (rc == 1 && ++attempt < MAX_ATTEMPTS) {
        uint32_t backoffMs = 1000UL << (attempt > 4 ? 4 : attempt);
        if (backoffMs > 30000) backoffMs = 30000;
        vTaskDelay(pdMS_TO_TICKS(backoffMs));
        continue;
      }
      if (rc == 1) setStatus(ChatState::FAILED, gStatus);
      break;  // 0 done, 2 fatal, or retries exhausted
    }
    gPendingSet = false;
  }
}

void claudeBegin() {
  gReqBuf = (char *)ps_malloc(REQ_BUF_BYTES);
  gAssistBuf = (char *)ps_malloc(ASSIST_BUF_BYTES);
  gTurnBuf = (char *)ps_malloc(TURN_BUF_BYTES);
  gToolInputs = (char *)ps_malloc(MAX_TOOLS * TOOL_INPUT_SLOT);
  gToolResult = (char *)ps_malloc(4096);
  gPending = (char *)ps_malloc(1024);
  gRing = (char *)ps_malloc(DELTA_RING_BYTES);
  char *arena = (char *)ps_malloc(CHAT_ARENA_BYTES);
  ChatTurn *turns = (ChatTurn *)ps_malloc(sizeof(ChatTurn) * CHAT_TURN_CAP);
  if (!gReqBuf || !gAssistBuf || !gTurnBuf || !gToolInputs || !gToolResult ||
      !gPending || !gRing || !arena || !turns) {
    Serial.println("claude: PSRAM alloc FAILED");
    return;
  }
  gLog.init(arena, CHAT_ARENA_BYTES, turns, CHAT_TURN_CAP);
  xTaskCreatePinnedToCore(netTask, "claude", 10240, nullptr, 4, nullptr, 0);
}
