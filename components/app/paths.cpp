#include "app/paths.h"

#include <vector>

namespace app::paths {

static AppPaths g_paths;

void set(AppPaths p) {
    // Normalize and store without trailing slashes.
    p.data  = ensure_no_trailing_slash(normalize(to_posix(p.data)));
    p.cache = ensure_no_trailing_slash(normalize(to_posix(p.cache)));
    p.logs  = ensure_no_trailing_slash(normalize(to_posix(p.logs)));
    g_paths = std::move(p);
}

const AppPaths& get() {
    return g_paths;
}

std::string to_posix(std::string_view p) {
    std::string out;
    out.reserve(p.size());
    for (char c : p) {
        out.push_back(c == '\\' ? '/' : c);
    }
    return out;
}

static void push_part(std::vector<std::string>& parts, std::string_view part) {
    if (part.empty() || part == ".") return;
    if (part == "..") {
        if (!parts.empty()) parts.pop_back();
        return;
    }
    parts.emplace_back(part);
}

std::string normalize(std::string_view p) {
    // POSIX normalize: collapse //, resolve . and .., preserve leading '/'.
    std::string s = to_posix(p);

    bool abs = !s.empty() && s[0] == '/';

    std::vector<std::string> parts;
    std::string_view sv{s};

    size_t i = 0;
    while (i < sv.size()) {
        while (i < sv.size() && sv[i] == '/') i++;
        size_t start = i;
        while (i < sv.size() && sv[i] != '/') i++;
        if (start < i) {
            push_part(parts, sv.substr(start, i - start));
        }
    }

    std::string out;
    if (abs) out.push_back('/');

    for (size_t k = 0; k < parts.size(); k++) {
        if (k > 0) out.push_back('/');
        out += parts[k];
    }

    // Special case: normalize("") -> ""
    // normalize("/") -> "/"
    if (abs && out.empty()) out = "/";

    return out;
}

std::string ensure_no_trailing_slash(std::string_view p) {
    std::string s = to_posix(p);
    while (s.size() > 1 && s.back() == '/') s.pop_back(); // keep "/" as-is
    return s;
}

std::string ensure_trailing_slash(std::string_view p) {
    std::string s = ensure_no_trailing_slash(p);
    if (s.empty()) return std::string{};
    if (s.back() != '/') s.push_back('/');
    return s;
}

std::string join(std::string_view a, std::string_view b) {
    if (a.empty()) return normalize(b);
    if (b.empty()) return normalize(a);

    std::string A = ensure_no_trailing_slash(a);
    std::string B = to_posix(b);

    // If B is absolute, treat it as authoritative.
    if (!B.empty() && B[0] == '/') return normalize(B);

    if (!A.empty() && A.back() != '/') A.push_back('/');
    A += B;
    return normalize(A);
}

bool is_absolute(std::string_view p) {
    return !p.empty() && p[0] == '/';
}

std::string dirname(std::string_view p) {
    std::string s = ensure_no_trailing_slash(to_posix(p));
    auto pos = s.find_last_of('/');
    if (pos == std::string::npos) return {};
    if (pos == 0) return "/"; // "/x" -> "/"
    return s.substr(0, pos);
}

std::string basename(std::string_view p) {
    std::string s = ensure_no_trailing_slash(to_posix(p));
    auto pos = s.find_last_of('/');
    if (pos == std::string::npos) return s;
    return s.substr(pos + 1);
}

std::string extname(std::string_view p) {
    std::string base = basename(p);
    auto dot = base.find_last_of('.');
    if (dot == std::string::npos || dot == 0) return {};
    return base.substr(dot);
}

std::string strip_extension(std::string_view p) {
    std::string s = to_posix(p);
    std::string base = basename(s);
    auto dot = base.find_last_of('.');
    if (dot == std::string::npos || dot == 0) return s;

    // Remove the extension portion in the original string.
    // Find last '/' in s and replace tail.
    auto slash = s.find_last_of('/');
    if (slash == std::string::npos) return base.substr(0, dot);
    return s.substr(0, slash + 1) + base.substr(0, dot);
}

std::string replace_extension(std::string_view p, std::string_view new_ext) {
    std::string s = strip_extension(p);
    if (new_ext.empty()) return s;

    std::string ext = std::string(new_ext);
    if (ext[0] != '.') ext.insert(ext.begin(), '.');
    return s + ext;
}

} // namespace app::paths
