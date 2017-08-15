/*
* ---------------------------------------------------------------------
* Copyright (c) 2017      Raheman Vaiya
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
* ---------------------------------------------------------------------
*/


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
struct xft_font_drw *xft_get_font_drw(Display *dpy,
                                      xcb_window_t win,
                                      const char *name,
                                      const char *color);
#endif
