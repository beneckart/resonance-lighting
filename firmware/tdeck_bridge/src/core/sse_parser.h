#pragma once

#include <stddef.h>
#include <stdint.h>

// Anthropic Messages-API SSE stream parser. Pure (no Arduino); fed decoded
// HTTP body bytes (the chunked-transfer framing is stripped by the caller).
// Native tests: tests/test_sse_parser.cpp.
//
// Events handled: content_block_start (text | tool_use), content_block_delta
// (text_delta -> onText, input_json_delta -> onToolInput), content_block_stop,
// message_delta (stop_reason), message_stop, error. Tool callbacks exist now
// so the M4 agent loop is a consumer change, not a parser change.

struct SseCallbacks {
  void (*onText)(void *cx, const char *utf8, size_t len);
  void (*onToolUseStart)(void *cx, const char *name, const char *toolUseId);
  void (*onToolInput)(void *cx, const char *jsonFragment, size_t len);
  void (*onBlockStop)(void *cx);
  void (*onStopReason)(void *cx, const char *reason);  // "end_turn","tool_use",...
  void (*onMessageStop)(void *cx);
  void (*onError)(void *cx, const char *message);
  void *cx;
};

class SseParser {
 public:
  void reset();
  // Feed any number of bytes; callbacks fire as complete lines assemble.
  void feed(const uint8_t *bytes, size_t n, const SseCallbacks &cb);

 private:
  void handleLine(const SseCallbacks &cb);
  enum { kLineCap = 4096, kNameCap = 64 };
  char mLine[kLineCap];
  size_t mLen = 0;
  bool mOverflow = false;
  char mEvent[kNameCap] = {0};
};

// Exposed for reuse/tests: decode a JSON string VALUE (starting after the
// opening quote) into out; returns bytes written, or -1 on malformed input.
// Handles \" \\ \/ \b \f \n \r \t and \uXXXX (BMP -> UTF-8; surrogate pairs
// unsupported -> '?').
int jsonUnescape(const char *src, size_t srcLen, char *out, size_t outCap,
                 size_t *consumed);

// Find `"key":"` in a JSON line and unescape its value. Returns length or -1.
int jsonFindString(const char *json, size_t jsonLen, const char *key,
                   char *out, size_t outCap);

// Find `"key":` followed by a bare number; returns dflt when absent.
long jsonFindInt(const char *json, size_t jsonLen, const char *key, long dflt);
