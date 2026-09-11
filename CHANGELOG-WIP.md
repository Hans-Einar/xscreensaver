# Work in progress

## 2026-09-11 — Phosphor dialog on XScreenSaver 6.16

- Import the existing local Phosphor unlock theme into a fork with upstream history.
- Compact rounded green frame, translucent black fill and opaque fallback.
- Large centered Matrix-font password mask and green CRT-style blinking block cursor.
- Cosmetic glyph generation uses input length, not password contents; readable
  PAM/status prompts and the original asterisk fallback remain available.
- Preserve existing authentication, input grabs and failure behavior; no PIN support.
- Add implementation/build notes, font/source provenance and the private-display test.

This is a development checkpoint, not a tagged binary release. See
[validation notes](docs/phosphor/README.md#validation) for tested scope and limits.
