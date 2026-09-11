# Phosphor unlock dialog

## Source and scope

Imported from the working local XScreenSaver 6.16 customization on 2026-09-11.
All six implementation files match the previously built source byte for byte:

- `driver/dialog.c`: theme selection, visual/lifecycle integration and redraw.
- `driver/dialog-phosphor.h`: compact layout and rounded translucent input field.
- `driver/dialog-matrix.h`: FreeType atlas, cosmetic masking and CRT-style cursor.
- `driver/dialog-matrix-font.h`: embedded Matrix-Code font.
- `driver/XScreenSaver.ad.in`: selectable Phosphor theme name.
- `driver/Makefile.in`: XRender linkage and header dependencies.

The original theme and splash/debug layouts remain available. The daemon’s
covering windows stay opaque. ARGB compositing affects the small prompt only.
No PAM configuration, authentication policy or login/boot service is included here.

FreeType builds a small A8 atlas of 57 glyphs once per helper. XRender draws the
glyphs and cursor; no OpenGL context or external font installation is required.
The mask uses character-count metadata, never password bytes. Inserting text
assigns cosmetic random glyphs; Backspace removes them. Long input shows a
centered suffix while retaining the full password buffer. Hiding asterisks still
hides input length. Atlas creation failure falls back to the original mask.

## Build

Use the upstream [README](../../README) and configure diagnostics for the full
XScreenSaver dependency list. This addition requires FreeType and XRender
headers/libraries (available through the local Xft development stack).
The 6.16 release ships a `configure` script with an unexpanded
`AM_GNU_GETTEXT(external)` macro. Regenerate it first using Autoconf, Automake
and gettext-devel/intltool (in addition to the upstream build dependencies).
These generated files are local build products; the fork retains the original
release copies in Git. The following out-of-tree build matches AlmaLinux 10.2:

```sh
aclocal
autoconf
mkdir -p build/phosphor
cd build/phosphor
../../configure --prefix=/usr/local --sysconfdir=/etc \
  --with-pam --with-systemd --with-login-manager=no --with-glx
make -C utils -j2
make -C driver -j2 xscreensaver-auth
```

This builds the auth helper only, not every bundled screensaver. No installation
or live screen locking is performed by these commands.

For the existing `/usr/local` installation, the changed installed files are
`/usr/local/libexec/xscreensaver/xscreensaver-auth` and
`/usr/share/X11/app-defaults/XScreenSaver` (from `driver/XScreenSaver.ad`).
Preserve the installation’s helper owner, mode, and SELinux context when replacing
it. The validated local installation uses root:root, mode 4755; this is not a
request to alter authentication or privilege policy on other installations.
Keep backups of both files before replacing them. For a new installation,
follow the upstream full build/install instructions and the distribution’s PAM setup.

After installation, select **Phosphor** in XScreenSaver’s settings or set
`dialogTheme: phosphor` in `~/.xscreensaver`. Reopen settings to load the new theme
name. The daemon launches the helper for each unlock attempt, so changing the
helper does not require restarting Window Maker. Choose `dialogTheme: default`
to return to the normal appearance.

## Validation

The import was checked against the pristine 6.16 release and the existing patched
source. A fresh out-of-tree build of `utils` and `xscreensaver-auth` from this fork
passed on 2026-09-11 with two build jobs and no compiler warnings. The tested helper
was not installed over the live installation during this repository import. Before import, the local build and private Xvfb tests passed with and
without a compositor: ordinary input, Backspace, long input, deletion to empty,
blinking/fading cursor, Escape and auth-helper termination. Escape and terminating
the helper both left the private daemon locked. No test submitted a password to
PAM; successful real-account authentication remains a manual check.

The included [test script](test-dialog.py) reproduces that local installed-layout
check. It requires Xvfb, xcompmgr, libXtst and `/usr/bin/python3` with GTK 3 GI,
plus XScreenSaver and `xmatrix` under `/usr/local`:

```sh
python3 docs/phosphor/test-dialog.py --daemon /usr/local/bin/xscreensaver
```

It creates private displays and a temporary HOME, uses disposable input, and
writes screenshots/logs under `/tmp/phosphor-*`. The live display is not used.
Run one copy at a time. This tests the **installed** helper selected by that
daemon, not an arbitrary build-tree helper. The composited path uses `xcompmgr -n`.
The original empty-cursor check sampled 22 frames, with green-channel brightness
5–190 opaque and 8–195 composited. These are local measurements, not portable
pixel-exact acceptance values. No endurance or multi-monitor test is claimed.

## Exact provenance

- Source base: [Zygo mirror commit b99f6216cd23](https://github.com/Zygo/xscreensaver/commit/b99f6216cd23),
  importing the [official 6.16 tarball](https://www.jwz.org/xscreensaver/xscreensaver-6.16.tar.gz).
- Tarball SHA-256: `91153c7c5b996761606dd7962c9f4bd4005a25874eac02776d0cf0026e6454e4`.
- Font: 7,896 bytes from `windows/Matrix-Code.ttf` in
  [Delmay441/Matrix-Reflow](https://github.com/Delmay441/Matrix-Reflow), also matching
  the embedded `windows/font_data.h` used by our Linux port.
- Font SHA-256: `5dff3023c91291d28dffc32fd74f5edbbca1969f25077d0499b1f000cabdec5b`.
- [Matrix-Reflow study](https://github.com/Hans-Einar/Matrix-Reflow/blob/main/Linux/study.md)
  records the unresolved third-party font licensing provenance. Embedding it here
  does not assert a new license or change upstream copyright notices.
