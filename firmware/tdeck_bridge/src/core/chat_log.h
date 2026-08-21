#pragma once

#include <stddef.h>
#include <stdint.h>

// Trimmed conversation window + request-body builder. Pure; storage injected
// (PSRAM on device, heap in tests). Roughly the last 12 exchanges are kept;
// oldest user/assistant pairs are evicted together so the window always
// starts with a user turn (API requirement).

enum ChatRole : uint8_t {
  CHAT_USER = 0,
  CHAT_ASSISTANT = 1,
  // RAW variants: the stored text is a pre-built JSON content ARRAY (tool_use
  // / tool_result blocks) embedded verbatim in the request instead of being
  // escaped as a plain string. The M4 agent loop uses these.
  CHAT_USER_RAW = 2,
  CHAT_ASSISTANT_RAW = 3,
};

struct ChatTurn {
  uint8_t role;
  uint32_t off;
  uint32_t len;
};

class ChatLog {
 public:
  void init(char *arena, size_t arenaCap, ChatTurn *turns, size_t turnCap);
  void clear();
  bool addTurn(uint8_t role, const char *text, size_t len);
  size_t turnCount() const { return mCount; }
  const char *turnText(size_t i, size_t *len, uint8_t *role) const;

  // Build the Messages-API request body. Returns length or -1 if cap is too
  // small. thinking disabled + effort low per the design brief (short operator
  // turns on a 320x240 screen). toolsJson: a JSON array for the "tools" field,
  // or nullptr for a plain chat request.
  int buildRequest(char *out, size_t cap, const char *model,
                   unsigned maxTokens, const char *systemPrompt,
                   const char *toolsJson) const;

 private:
  void evictOldestPair();
  char *mArena = nullptr;
  size_t mArenaCap = 0;
  ChatTurn *mTurns = nullptr;
  size_t mTurnCap = 0;
  size_t mCount = 0;
  size_t mUsed = 0;
};

// Append a JSON-escaped copy of src to out at offset o (capacity cap).
// Returns new offset or -1 on overflow. Shared by chat_log and the M4 agent.
int jsonEscapeAppend(char *out, size_t cap, int o, const char *src, size_t n);
