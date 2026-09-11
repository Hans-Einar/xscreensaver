/* Compact local unlock theme. Included by dialog.c, using its existing input
   handling and PAM conversation. This file only paints the prompt. */

#include "dialog-matrix.h"

static void
phosphor_round_rect (Display *dpy, Drawable d, GC gc,
                     int x, int y, int w, int h, int r)
{
  XFillRectangle (dpy, d, gc, x+r, y, w-2*r, h);
  XFillRectangle (dpy, d, gc, x, y+r, w, h-2*r);
  XFillArc (dpy, d, gc, x, y, 2*r, 2*r, 0, 360*64);
  XFillArc (dpy, d, gc, x+w-2*r, y, 2*r, 2*r, 0, 360*64);
  XFillArc (dpy, d, gc, x, y+h-2*r, 2*r, 2*r, 0, 360*64);
  XFillArc (dpy, d, gc, x+w-2*r, y+h-2*r, 2*r, 2*r, 0, 360*64);
}

static void
phosphor_window_draw (window_state *ws)
{
  Display *dpy = ws->dpy;
  XWindowAttributes attr;
  XftColor green, muted, error;
  XftDraw *draw;
  XGlyphInfo ext;
  Pixmap buffer;
  GC gc;
  /* Keep text and outline opaque; only the black fill has reduced alpha. */
  unsigned long bg = ws->argb_p ? 0xc4000000UL : 0;
  int pad = 22, vpad = 6, radius = 9, y = vpad, j, n = 0;
  int line_height = MAX (MATRIX_H, ws->label_font->ascent + ws->label_font->descent);
  int width = MAX (420, (ws->label_font->ascent + ws->label_font->descent) * 18);
  int height, input_y, input_count = 0;
  dialog_line *notices = calloc (ws->nmsgs + 5, sizeof (*notices));
  char **owned = calloc (ws->nmsgs + 5, sizeof (*owned));

  if (!notices || !owned) abort();
  width = MIN (width, WidthOfScreen (ws->screen) - 40);
  XftColorAllocName (dpy, ws->dialog_visual, ws->dialog_cmap, "#a8e65d", &green);
  XftColorAllocName (dpy, ws->dialog_visual, ws->dialog_cmap, "#729745", &muted);
  XftColorAllocName (dpy, ws->dialog_visual, ws->dialog_cmap, "#ef8975", &error);

  /* Retain error and nonstandard PAM messages (e.g. expired passwords and
     additional factors). Only the ordinary password label is redundant. */
#define NOTICE(S, COLOR) do { \
    owned[n] = xft_word_wrap (dpy, ws->body_font, (S), width - pad*2); \
    notices[n].text = owned[n]; \
    notices[n].font = ws->body_font; \
    notices[n].fg = notices[n].fg2 = (COLOR); \
    notices[n].bg = bg; \
    notices[n].type = LABEL; notices[n].align = LEFT; \
    n++; \
  } while (0)

  if (ws->body_label && *ws->body_label)
    NOTICE (ws->body_label, error);
  for (j = 0; j < ws->nmsgs; j++)
    {
      const char *msg = ws->msgs[j].msg;
      Bool prompt = (ws->msgs[j].type == AUTH_MSGTYPE_PROMPT_NOECHO ||
                     ws->msgs[j].type == AUTH_MSGTYPE_PROMPT_ECHO);
      if (prompt) input_count++;
      if (msg && *msg &&
          !(ws->msgs[j].type == AUTH_MSGTYPE_PROMPT_NOECHO &&
            (!strcmp (msg, "Password:") || !strcmp (msg, "Password: ") ||
             !strcmp (msg, _("Password:")))))
        NOTICE (msg, ws->msgs[j].type == AUTH_MSGTYPE_ERROR ? error : muted);
    }
  if (ws->caps_p) NOTICE (_("Caps Lock"), error);
  if (time (0) - XSCREENSAVER_RELEASED > 60*60*24*30*17)
    NOTICE (_("Update available!\nThis version is very old.\n"), error);
#undef NOTICE

  for (j = 0; j < n; j++)
    y += XftTextExtentsUtf8_multi (dpy, ws->body_font,
                                  (FcChar8 *) notices[j].text,
                                  strlen (notices[j].text), &ext) *
         (ws->body_font->ascent + ws->body_font->descent);
  input_y = y;
  if (n && input_count) input_y += 10;
  height = input_y + input_count * (line_height + 10) - (input_count ? 10 : 0) + vpad;
  height = MAX (height, line_height + vpad*2);
  height = MAX (height, ws->min_height);
  ws->min_height = height;

  XGetWindowAttributes (dpy, ws->window, &attr);
  if (!ws->x)
    {
      ws->x = ws->cx - width / 2;
      ws->y = ws->cy - height / 2;
    }
  if (attr.width != width || attr.height != height ||
      attr.x != ws->x || attr.y != ws->y ||
      window_occluded_p (dpy, ws->window))
    {
      create_window (ws, width, height);
      XMapRaised (dpy, ws->window);
    }

  buffer = XCreatePixmap (dpy, ws->window, width, height, ws->dialog_depth);
  gc = XCreateGC (dpy, buffer, 0, 0);
  XSetForeground (dpy, gc, 0);
  XFillRectangle (dpy, buffer, gc, 0, 0, width, height);
  XSetForeground (dpy, gc, green.pixel);
  phosphor_round_rect (dpy, buffer, gc, 0, 0, width, height, radius);
  XSetForeground (dpy, gc, bg);
  phosphor_round_rect (dpy, buffer, gc, 2, 2, width-4, height-4, radius-2);

  draw = XftDrawCreate (dpy, buffer, ws->dialog_visual, ws->dialog_cmap);
  matrix_build_atlas (ws);
  matrix_sync_count (ws);
  y = vpad;
  for (j = 0; j < n; j++)
    {
      XftDrawStringUtf8_multi (draw, &notices[j].fg, ws->body_font,
                               pad, y + ws->body_font->ascent,
                               (FcChar8 *) notices[j].text,
                               strlen (notices[j].text), 1);
      y += XftTextExtentsUtf8_multi (dpy, ws->body_font,
                                    (FcChar8 *) notices[j].text,
                                    strlen (notices[j].text), &ext) *
           (ws->body_font->ascent + ws->body_font->descent);
    }
  y = input_y;
  for (j = 0; j < ws->nmsgs; j++)
    {
      const char *text;
      int x, caret;
      XRectangle clip;
      if (ws->msgs[j].type != AUTH_MSGTYPE_PROMPT_NOECHO &&
          ws->msgs[j].type != AUTH_MSGTYPE_PROMPT_ECHO) continue;
      if (ws->msgs[j].type == AUTH_MSGTYPE_PROMPT_NOECHO &&
          ws->auth_state != AUTH_FINISHED && ws->matrix_mask)
        {
          unsigned int count = ws->show_stars_p ? ws->matrix_count : 0;
          unsigned int max_visible = MAX (0, (width-pad*2)/MATRIX_W-2);
          unsigned int visible = MIN (count, max_visible), k;
          int total = MAX (1, visible)*MATRIX_W;
          Picture dst = XftDrawPicture (draw);
          x = (width-total)/2;
          for (k=0; k<visible; k++)
            matrix_glyph (ws, dst, ws->matrix_symbols[count-visible+k],
                           x+k*MATRIX_W, y, 1);
          matrix_cursor (ws, dst, x+(visible ? visible*MATRIX_W : 0)+3, y, double_time());
          matrix_scanlines (ws, dst, x-3, y, total+MATRIX_W+6);
        }
      else
        {
          /* Keep echo-enabled PAM prompts and status text readable. A failed
             atlas load falls back to the existing asterisk mask. */
          text = (ws->auth_state == AUTH_FINISHED ? _("Checking...") :
                  ws->msgs[j].type == AUTH_MSGTYPE_PROMPT_ECHO ? ws->plaintext_passwd :
                  ws->show_stars_p ? ws->censored_passwd : "");
          XftTextExtentsUtf8 (dpy, ws->label_font, (FcChar8 *) text,
                              strlen (text), &ext);
          x = MAX (pad, (width-ext.xOff)/2);
          if (ext.xOff > width-pad*2) x = width-pad-ext.xOff;
          clip.x = pad; clip.y = y; clip.width = width-pad*2;
          clip.height = line_height + 2;
          XftDrawSetClipRectangles (draw, 0, 0, &clip, 1);
          XftDrawStringUtf8 (draw, &green, ws->label_font,
                             x, y+(line_height-ws->label_font->height)/2+ws->label_font->ascent,
                             (FcChar8 *) text, strlen (text));
          XftDrawSetClip (draw, 0);
          if (ws->i_beam && ws->auth_state != AUTH_FINISHED)
            {
              caret = MIN (width-pad-2, MAX (pad, x+ext.xOff+2));
              XSetForeground (dpy, gc, green.pixel);
              XFillRectangle (dpy, buffer, gc, caret, y+2, 2, line_height-2);
            }
        }
      y += line_height + 10;
    }
  XCopyArea (dpy, buffer, ws->window, gc, 0, 0, width, height, 0, 0);
  XSync (dpy, False);
  XftDrawDestroy (draw);
  XFreeGC (dpy, gc);
  XFreePixmap (dpy, buffer);
  for (j = 0; j < n; j++) free (owned[j]);
  free (owned);
  free (notices);
  XftColorFree (dpy, ws->dialog_visual, ws->dialog_cmap, &green);
  XftColorFree (dpy, ws->dialog_visual, ws->dialog_cmap, &muted);
  XftColorFree (dpy, ws->dialog_visual, ws->dialog_cmap, &error);
}
