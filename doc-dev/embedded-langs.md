# Embeddable Scripting Languages

| Lang | Files | Dependencies | Footprint | License | Notes |
|------|-------|-------------|-----------|---------|-------|
| **s7 Scheme** (current) | 2 (s7.c, s7.h) | None | ~4 MB src, ~300 KB obj | Public domain | Zero-dependency amalgamation. Minimal API. |
| **Lua** | ~25 .c/.h | None | ~200–300 KB | MIT | Gold standard. Huge ecosystem. Stack API. |
| **Duktape** (JS) | 2 (duktape.c, duktape.h) | None | ~300–600 KB | MIT | Same 2-file drop-in as s7. ES5. |
| **QuickJS** (JS) | ~5 files | None | ~1–1.5 MB | MIT | ES2020. Fast. Heavier. |
| **Wren** | 2 (wren.c, wren.h) | None | ~150–250 KB | MIT | Smallest footprint. Clean API. Weak ecosystem. |
| **ChaiScript** | Header-only | C++ stdlib | ~200 KB (bloat) | BSD | Easiest embed. Slow eval. |
| **mruby** | ~30–50 | None | ~1–1.5 MB | MIT | Heavy object model. Not great for audio-rate. |

For `ra-scheme`, the main argument to switch would be **syntax familiarity** (Lua or JS), not performance or footprint.
