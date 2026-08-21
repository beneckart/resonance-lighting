#pragma once

#include <stddef.h>

// Execute one agent tool against the census/mesh. Runs on the claude net
// task; every mesh effect goes through mesh_tx (the single Nb sender) and
// fleet-wide actions block on the on-device confirm rail.
// Returns false -> the result string is an error message (tool_result gets
// is_error:true) so the model hears an honest refusal instead of silence.
bool agentExecuteTool(const char *name, const char *inputJson, size_t inputLen,
                      char *result, size_t resultCap);
