#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

#include "core/chat_log.h"

int main() {
  static char arena[512];
  static ChatTurn turns[6];
  ChatLog log;
  log.init(arena, sizeof(arena), turns, 6);

  assert(log.addTurn(CHAT_USER, "hi", 2));
  assert(log.addTurn(CHAT_ASSISTANT, "hello \"tree\"\n", 13));
  assert(log.turnCount() == 2);

  char out[1024];
  int n = log.buildRequest(out, sizeof(out), "claude-sonnet-5", 900,
                           "You are the tree console.", nullptr);
  assert(n > 0);
  std::string body(out);
  assert(body.find("\"model\":\"claude-sonnet-5\"") != std::string::npos);
  assert(body.find("\"stream\":true") != std::string::npos);
  assert(body.find("\"thinking\":{\"type\":\"disabled\"}") != std::string::npos);
  assert(body.find("\"role\":\"user\",\"content\":\"hi\"") != std::string::npos);
  // Escapes survive the round trip.
  assert(body.find("hello \\\"tree\\\"\\n") != std::string::npos);
  // Crude structural sanity: balanced braces/brackets outside strings.
  int depth = 0;
  bool instr = false, esc = false;
  for (char c : body) {
    if (instr) {
      if (esc) { esc = false; continue; }
      if (c == '\\') { esc = true; continue; }
      if (c == '"') instr = false;
      continue;
    }
    if (c == '"') instr = true;
    else if (c == '{' || c == '[') ++depth;
    else if (c == '}' || c == ']') --depth;
  }
  assert(depth == 0);

  // --- pair eviction keeps the window user-first and within capacity ---
  ChatLog small;
  static char sArena[96];
  static ChatTurn sTurns[4];
  small.init(sArena, sizeof(sArena), sTurns, 4);
  assert(small.addTurn(CHAT_USER, "u1", 2));
  assert(small.addTurn(CHAT_ASSISTANT, "a1", 2));
  assert(small.addTurn(CHAT_USER, "u2", 2));
  assert(small.addTurn(CHAT_ASSISTANT, "a2", 2));
  assert(small.addTurn(CHAT_USER, "u3", 2));  // evicts u1/a1
  assert(small.turnCount() == 3);
  uint8_t role;
  size_t len;
  const char *t0 = small.turnText(0, &len, &role);
  assert(role == CHAT_USER && strncmp(t0, "u2", 2) == 0);

  // Arena overflow evicts rather than corrupting.
  static char big[80];
  memset(big, 'x', sizeof(big));
  assert(small.addTurn(CHAT_ASSISTANT, big, sizeof(big)));
  assert(small.turnCount() >= 1);

  // RAW turns embed verbatim content arrays and tools ride the header.
  ChatLog rawLog;
  static char rArena[512];
  static ChatTurn rTurns[4];
  rawLog.init(rArena, sizeof(rArena), rTurns, 4);
  assert(rawLog.addTurn(CHAT_USER, "list quiet nodes", 16));
  const char *araw =
      "[{\"type\":\"tool_use\",\"id\":\"t1\",\"name\":\"mesh_census\","
      "\"input\":{\"quiet_s\":60}}]";
  assert(rawLog.addTurn(CHAT_ASSISTANT_RAW, araw, strlen(araw)));
  const char *uraw =
      "[{\"type\":\"tool_result\",\"tool_use_id\":\"t1\",\"content\":\"{}\"}]";
  assert(rawLog.addTurn(CHAT_USER_RAW, uraw, strlen(uraw)));
  n = rawLog.buildRequest(out, sizeof(out), "m", 100, "s",
                          "[{\"name\":\"mesh_census\"}]");
  assert(n > 0);
  std::string rb(out);
  assert(rb.find("\"tools\":[{\"name\":\"mesh_census\"}]") != std::string::npos);
  assert(rb.find("\"content\":[{\"type\":\"tool_use\"") != std::string::npos);
  assert(rb.find("\"content\":[{\"type\":\"tool_result\"") != std::string::npos);
  assert(rb.find("\"role\":\"assistant\",\"content\":[") != std::string::npos);

  printf("chat_log ok\n");
  return 0;
}
