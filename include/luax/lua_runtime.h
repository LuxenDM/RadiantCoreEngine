#pragma once

// Returns true if the script ran successfully, false if Lua error (or invalid args).
bool luax_run_file(const char* path);
