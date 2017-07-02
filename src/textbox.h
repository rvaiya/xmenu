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


#ifndef _TEXTBOX_H_
#define _TEXTBOX_H_

struct textbox {
  Display *dpy;
  xcb_connection_t *con;
  xcb_window_t win;
  xcb_pixmap_t pm;
  xcb_pixmap_t opm;
  xcb_gcontext_t curgc;
  xcb_gcontext_t gc;
  uint32_t x,y;
  /* boolean indicating whether or not to fill the input background with the background color. */
  int fill; 
  int width;
  int height;
  const char *fontname;
  struct xft_font_drw *fdrw;
};

char *textbox_query(struct textbox *ctx);
size_t textbox_height(Display *dpy, const char *font);
struct textbox *textbox_init(Display *dpy, int x, int y, const char *fgcol, const char *bgcol,
                             const char *font, int width, int fill);
#endif
