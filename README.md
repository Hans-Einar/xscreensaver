# XScreenSaver — Phosphor / Matrix unlock dialog

This is [Hans-Einar’s customization](https://github.com/Hans-Einar/xscreensaver)
of **XScreenSaver 6.16**, adding an optional compact green password dialog for
our X11 / Window Maker desktop.

Select **Phosphor** as the dialog theme in `xscreensaver-settings`, or set
`dialogTheme: phosphor` in `~/.xscreensaver` after building and installing.
The dialog has a translucent black background, a rounded green outline,
large centered random Matrix glyphs and a blinking block cursor with a CRT-like
fade, glow and scanlines. Without a compositor, the background is opaque black.

The existing XScreenSaver authentication flow remains responsible for PAM,
keyboard grabs, password handling and authentication results. This customization
changes the prompt rendering; it does not add PIN authentication. Normal PAM
messages remain readable, and the default theme is still available.

- [Build, installation, implementation and test notes](docs/phosphor/README.md)
- [Work-in-progress changes](CHANGELOG-WIP.md)
- [Original upstream README and release history](README)
- [Matrix Reflow Linux screensaver](https://github.com/Hans-Einar/Matrix-Reflow)
  — a separate effect, usable with or without this dialog theme

## Upstream and attribution

**XScreenSaver is by Jamie Zawinski and many contributors.** Its authoritative
project and releases are at [jwz.org/xscreensaver](https://www.jwz.org/xscreensaver/).
This repository is a GitHub fork of [Zygo/xscreensaver](https://github.com/Zygo/xscreensaver),
a third-party read-only mirror of those releases, not the official project.
`master` retains the mirror’s release history; `main` carries our customization.
Report problems specific to this theme to this fork.

The embedded font was copied from
[Delmay441/Matrix-Reflow](https://github.com/Delmay441/Matrix-Reflow), via our
[Linux port](https://github.com/Hans-Einar/Matrix-Reflow).
Matrix-Reflow is based on
[DigitalChewie/ModernMatrixScreensaver](https://github.com/DigitalChewie/ModernMatrixScreensaver)
and credits [Rezmason/matrix](https://github.com/Rezmason/matrix) for the font
and visual inspiration. See the provenance notes below for the exact bytes/base.

Existing upstream copyright and permission notices are retained. The Matrix font
is a separate inherited asset; its redistribution license has not been independently
established, and no new license is asserted for it.
