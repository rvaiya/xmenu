#ifndef _XFT_H__
#define _XFT_H__
#include <X11/Xft/Xft.h>
#include <xcb/xcb.h>
#include "util.h"

/* Font drawable created by xft_get_font and used by the rest of the
   xft functions as context for drawing operations. */

struct xft_font_drw {
  Display *dpy;
  XftFont *font;
  xcb_window_t win;
  XftColor *col;
  XftDraw *drw;
  uint32_t maxheight;
};

struct geom xft_text_geom(struct xft_font_drw *font, const char *str, size_t len);
void xft_draw_text(struct xft_font_drw *font,
                   int x, int y,
                   const char *str,
                   size_t len);
void xft_free(struct xft_font_drw *font);
struct xft_font_drw *xft_get_font_drw(Display *dpy, xcb_window_t win, const char *name, const char *color);
#endif
