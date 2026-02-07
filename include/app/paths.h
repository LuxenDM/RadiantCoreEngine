#pragma once
#include <string>
#include <string_view>

namespace app::paths {

// App-facing semantic directories.
// All strings are stored in POSIX style ('/' separators), no trailing slash.
struct AppPaths {
    std::string data;   // persistent app data (scripts, configs, saves, etc.)
    std::string cache;  // disposable cache
    std::string logs;   // logs (if you want a dedicated folder)
};

// ---- lifecycle ----
// Platform layer should call set() once early.
void set(AppPaths p);
const AppPaths& get();

// ---- path utils (portable, POSIX-style) ----
std::string to_posix(std::string_view p);              // converts '\' to '/'
std::string normalize(std::string_view p);             // resolves "." and "..", collapses "//"
std::string join(std::string_view a, std::string_view b);
std::string ensure_no_trailing_slash(std::string_view p);
std::string ensure_trailing_slash(std::string_view p);

bool is_absolute(std::string_view p);                  // POSIX meaning: begins with '/'
std::string dirname(std::string_view p);               // "a/b/c.txt" -> "a/b"
std::string basename(std::string_view p);              // "a/b/c.txt" -> "c.txt"
std::string extname(std::string_view p);               // "c.txt" -> ".txt" , "c" -> ""
std::string strip_extension(std::string_view p);       // "c.txt" -> "c"
std::string replace_extension(std::string_view p, std::string_view new_ext); // new_ext may include '.' or not

} // namespace app::paths
