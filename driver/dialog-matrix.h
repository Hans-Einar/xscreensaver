/* Rendering-only Matrix mask. No password bytes enter this code. */
#include "dialog-matrix-font.h"
#define MATRIX_W 38
#define MATRIX_H 48
static const unsigned long matrix_codes[] = {
  0x0022,0x002A,0x002B,0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0037,
  0x0038,0x0039,0x003A,0x003C,0x003E,0x007A,0x007C,0x00A6,0x00A9,
  0x254C,0x25AA,0x30A2,0x30A6,0x30A8,0x30AA,0x30AB,0x30AD,0x30B1,
  0x30B3,0x30B5,0x30B7,0x30B9,0x30BB,0x30BD,0x30BF,0x30C4,0x30C6,
  0x30CA,0x30CB,0x30CC,0x30CD,0x30CF,0x30D2,0x30DB,0x30DE,0x30DF,
  0x30E0,0x30E1,0x30E2,0x30E4,0x30E8,0x30E9,0x30EA,0x30EF,0x30FC,
  0xA78A,0xE937
};

static void
matrix_sync_count (window_state *ws)
{
  unsigned int n = strlen (ws->plaintext_passwd_char_size), i;
  if (!ws->matrix_rng)
    {
      if (getrandom (&ws->matrix_rng, sizeof(ws->matrix_rng), GRND_NONBLOCK)
          != sizeof(ws->matrix_rng))
        ws->matrix_rng = (uint32_t) (double_time() * 1000000) ^ getpid();
      if (!ws->matrix_rng) ws->matrix_rng = 0x9E3779B9;
    }
  n = MIN (n, countof(ws->matrix_symbols));
  for (i = ws->matrix_count; i < n; i++)
    {
      /* Private cosmetic PRNG: never seeded from input or shared with PAM. */
      uint32_t r = ws->matrix_rng;
      r ^= r << 13; r ^= r >> 17; r ^= r << 5; ws->matrix_rng = r;
      ws->matrix_symbols[i] = r % countof(matrix_codes);
    }
  if (n < ws->matrix_count)
    memset (ws->matrix_symbols+n, 0, ws->matrix_count-n);
  ws->matrix_count = n;
}

static void
matrix_build_atlas (window_state *ws)
{
  FT_Library library = 0;
  FT_Face face = 0;
  XImage *image = 0;
  GC gc;
  unsigned int i;
  int w = MATRIX_W * countof(matrix_codes);
  if (ws->matrix_initialized) return;
  ws->matrix_initialized = True;
  if (FT_Init_FreeType (&library) ||
      FT_New_Memory_Face (library, Matrix_Code_ttf, Matrix_Code_ttf_len, 0, &face) ||
      FT_Select_Charmap (face, FT_ENCODING_UNICODE) ||
      FT_Set_Pixel_Sizes (face, 0, 64)) goto DONE;
  image = XCreateImage (ws->dpy, ws->dialog_visual, 8, ZPixmap, 0, 0,
                        w, MATRIX_H, 8, 0);
  if (!image) goto DONE;
  image->data = calloc (image->bytes_per_line, MATRIX_H);
  if (!image->data) goto DONE;
  for (i = 0; i < countof(matrix_codes); i++)
    {
      FT_Bitmap *b;
      int x, y, gw, gh, ox, oy;
      double scale;
      FT_UInt index = FT_Get_Char_Index (face, matrix_codes[i]);
      if (!index || FT_Load_Glyph (face, index, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL)) goto DONE;
      b = &face->glyph->bitmap;
      if (!b->width || !b->rows || b->pixel_mode != FT_PIXEL_MODE_GRAY) goto DONE;
      scale = MIN ((MATRIX_W-4.0)/b->width, (MATRIX_H-4.0)/b->rows);
      gw = MAX (1, (int) (b->width * scale)); gh = MAX (1, (int) (b->rows * scale));
      ox = i*MATRIX_W + (MATRIX_W-gw)/2; oy = (MATRIX_H-gh)/2;
      for (y=0; y<gh; y++) for (x=0; x<gw; x++)
        {
          int sx = MIN ((int)(x/scale), b->width-1);
          int sy = MIN ((int)(y/scale), b->rows-1);
          const unsigned char *row = b->pitch >= 0 ? b->buffer+sy*b->pitch :
                                    b->buffer+(b->rows-1-sy)*(-b->pitch);
          XPutPixel (image, ox+x, oy+y, row[sx]);
        }
    }
  ws->matrix_atlas = XCreatePixmap (ws->dpy, ws->window, w, MATRIX_H, 8);
  gc = XCreateGC (ws->dpy, ws->matrix_atlas, 0, 0);
  XPutImage (ws->dpy, ws->matrix_atlas, gc, image, 0, 0, 0, 0, w, MATRIX_H);
  XFreeGC (ws->dpy, gc);
  ws->matrix_mask = XRenderCreatePicture (ws->dpy, ws->matrix_atlas,
                      XRenderFindStandardFormat (ws->dpy, PictStandardA8), 0, 0);
 DONE:
  if (image) XDestroyImage (image);
  if (face) FT_Done_Face (face);
  if (library) FT_Done_FreeType (library);
}

static void
matrix_glyph (window_state *ws, Picture dst, unsigned int glyph, int x, int y,
              double brightness)
{
  XRenderColor color = { 0x3600, 0xffff, 0x5900, 0xffff };
  Picture ink;
  int dx, dy;
  /* Dim offsets form a small phosphor halo without an OpenGL context. */
  color.alpha = 6000 * brightness;
  color.red = color.alpha * .21; color.green = color.alpha; color.blue = color.alpha * .35;
  ink = XRenderCreateSolidFill (ws->dpy, &color);
  for (dy=-1;dy<=1;dy++) for (dx=-1;dx<=1;dx++)
    if (dx || dy)
      XRenderComposite (ws->dpy, PictOpOver, ink, ws->matrix_mask, dst,
                         0, 0, glyph*MATRIX_W, 0, x+dx, y+dy, MATRIX_W, MATRIX_H);
  XRenderFreePicture (ws->dpy, ink);
  color.alpha = 65535 * brightness;
  color.red = color.alpha * .21; color.green = color.alpha; color.blue = color.alpha * .35;
  ink = XRenderCreateSolidFill (ws->dpy, &color);
  XRenderComposite (ws->dpy, PictOpOver, ink, ws->matrix_mask, dst,
                     0, 0, glyph*MATRIX_W, 0, x, y, MATRIX_W, MATRIX_H);
  XRenderFreePicture (ws->dpy, ink);
}

static void
matrix_cursor (window_state *ws, Picture dst, int x, int y, double now)
{
  double phase = now - (long) now;
  double level = phase < .55 ? 1 : phase < .78 ? ( .78-phase )/.23 : .03;
  XRenderColor color = { 0x3800, 0xffff, 0x5900, 0 };
  int i;
  for (i=3; i>0; i--)
    {
      color.alpha = (unsigned short) (2500 * level);
      color.red = color.alpha * .21; color.green = color.alpha; color.blue = color.alpha * .35;
      XRenderFillRectangle (ws->dpy, PictOpOver, dst, &color,
                            x-i, y-i, MATRIX_W-6+2*i, MATRIX_H-2+2*i);
    }
  color.alpha = (unsigned short) (52000 * level);
  color.red = color.alpha * .21; color.green = color.alpha; color.blue = color.alpha * .35;
  XRenderFillRectangle (ws->dpy, PictOpOver, dst, &color,
                        x, y+1, MATRIX_W-6, MATRIX_H-2);
}

static void
matrix_scanlines (window_state *ws, Picture dst, int x, int y, int w)
{
  XRenderColor dark = {0, 0, 0, 17000};
  int row;
  for (row=1; row<MATRIX_H; row+=3)
    XRenderFillRectangle (ws->dpy, PictOpOver, dst, &dark, x, y+row, w, 1);
}
