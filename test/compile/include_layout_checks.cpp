// Compile-only checks: zuru-ranges::zuru-ranges puts include/ on the include path, not the repository root,
// so a consumer reaches zr/... and nothing else of the source tree.
// Built as an OBJECT library, never run.
#if !__has_include("zr/ranges.h")
#error "zr/ranges.h not reachable through zuru-ranges::zuru-ranges"
#endif
#if __has_include("modules/zr.cppm") || __has_include("include/zr/ranges.h")
#error "the repository root is on the include path"
#endif
