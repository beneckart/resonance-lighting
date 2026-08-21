#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

#include "core/sse_parser.h"

struct Got {
  std::string text;
  std::string toolName, toolId, toolInput;
  std::string stopReason, error;
  int blockStops = 0;
  bool messageStop = false;
};

static SseCallbacks cbs(Got *g) {
  SseCallbacks cb = {};
  cb.cx = g;
  cb.onText = [](void *cx, const char *t, size_t n) {
    ((Got *)cx)->text.append(t, n);
  };
  cb.onToolUseStart = [](void *cx, const char *name, const char *id) {
    ((Got *)cx)->toolName = name;
    ((Got *)cx)->toolId = id;
  };
  cb.onToolInput = [](void *cx, const char *f, size_t n) {
    ((Got *)cx)->toolInput.append(f, n);
  };
  cb.onBlockStop = [](void *cx) { ++((Got *)cx)->blockStops; };
  cb.onStopReason = [](void *cx, const char *r) { ((Got *)cx)->stopReason = r; };
  cb.onMessageStop = [](void *cx) { ((Got *)cx)->messageStop = true; };
  cb.onError = [](void *cx, const char *m) { ((Got *)cx)->error = m; };
  return cb;
}

// Feed one byte at a time to prove reassembly across arbitrary boundaries.
static void feedSlow(SseParser &p, const char *s, const SseCallbacks &cb) {
  for (size_t i = 0; i < strlen(s); ++i) p.feed((const uint8_t *)s + i, 1, cb);
}

int main() {
  // --- unescape unit checks ---
  char buf[64];
  const char *src = "he\\nllo \\\"x\\\" \\u00e9\"tail";
  int n = jsonUnescape(src, strlen(src), buf, sizeof(buf), nullptr);
  assert(n >= 0);
  buf[n] = 0;
  assert(strcmp(buf, "he\nllo \"x\" \xc3\xa9") == 0);

  // --- a realistic streaming transcript ---
  Got g;
  SseCallbacks cb = cbs(&g);
  SseParser p;
  p.reset();
  feedSlow(p,
           "event: message_start\r\n"
           "data: {\"type\":\"message_start\",\"message\":{\"id\":\"msg_x\"}}\r\n"
           "\r\n"
           "event: content_block_start\r\n"
           "data: {\"type\":\"content_block_start\",\"index\":0,"
           "\"content_block\":{\"type\":\"text\",\"text\":\"\"}}\r\n\r\n"
           "event: content_block_delta\r\n"
           "data: {\"type\":\"content_block_delta\",\"index\":0,"
           "\"delta\":{\"type\":\"text_delta\",\"text\":\"Hello\"}}\r\n\r\n"
           "event: content_block_delta\r\n"
           "data: {\"type\":\"content_block_delta\",\"index\":0,"
           "\"delta\":{\"type\":\"text_delta\",\"text\":\", tree \\u2728\"}}\r\n\r\n"
           "event: content_block_stop\r\n"
           "data: {\"type\":\"content_block_stop\",\"index\":0}\r\n\r\n"
           "event: message_delta\r\n"
           "data: {\"type\":\"message_delta\",\"delta\":{\"stop_reason\":"
           "\"end_turn\"},\"usage\":{\"output_tokens\":9}}\r\n\r\n"
           "event: message_stop\r\n"
           "data: {\"type\":\"message_stop\"}\r\n\r\n",
           cb);
  assert(g.text == "Hello, tree \xe2\x9c\xa8");
  assert(g.blockStops == 1);
  assert(g.stopReason == "end_turn");
  assert(g.messageStop);
  assert(g.error.empty());

  // --- tool use assembly (M4 consumer contract) ---
  Got t;
  SseCallbacks tcb = cbs(&t);
  SseParser p2;
  p2.reset();
  feedSlow(p2,
           "event: content_block_start\n"
           "data: {\"type\":\"content_block_start\",\"index\":1,"
           "\"content_block\":{\"type\":\"tool_use\",\"id\":\"toolu_01\","
           "\"name\":\"mesh_census\",\"input\":{}}}\n\n"
           "event: content_block_delta\n"
           "data: {\"type\":\"content_block_delta\",\"index\":1,"
           "\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":"
           "\"{\\\"quiet\"}}\n\n"
           "event: content_block_delta\n"
           "data: {\"type\":\"content_block_delta\",\"index\":1,"
           "\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":"
           "\"_s\\\": 60}\"}}\n\n"
           "event: content_block_stop\n"
           "data: {\"type\":\"content_block_stop\",\"index\":1}\n\n"
           "event: message_delta\n"
           "data: {\"type\":\"message_delta\",\"delta\":{\"stop_reason\":"
           "\"tool_use\"}}\n\n",
           tcb);
  assert(t.toolName == "mesh_census");
  assert(t.toolId == "toolu_01");
  assert(t.toolInput == "{\"quiet_s\": 60}");
  assert(t.stopReason == "tool_use");

  // --- model-authored tool inputs use spaces after colons (regression:
  // this truncated/failed 6-hex fixture ids on the live device) ---
  const char *spaced = "{\"id\": \"9E5AF0\", \"secs\": 6, \"color\": \"green\"}";
  char idBuf[16];
  int idLen = jsonFindString(spaced, strlen(spaced), "id", idBuf, sizeof(idBuf));
  assert(idLen == 6);
  idBuf[idLen] = 0;
  assert(strcmp(idBuf, "9E5AF0") == 0);
  assert(jsonFindInt(spaced, strlen(spaced), "secs", -1) == 6);
  char colorBuf[12];
  int cl = jsonFindString(spaced, strlen(spaced), "color", colorBuf, sizeof(colorBuf));
  assert(cl == 5);

  // --- API error event ---
  Got e;
  SseCallbacks ecb = cbs(&e);
  SseParser p3;
  p3.reset();
  feedSlow(p3,
           "event: error\n"
           "data: {\"type\":\"error\",\"error\":{\"type\":\"overloaded_error\","
           "\"message\":\"Overloaded\"}}\n\n",
           ecb);
  assert(e.error == "Overloaded");

  printf("sse_parser ok\n");
  return 0;
}
