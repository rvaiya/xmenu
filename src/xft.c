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


#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>
#include <stdint.h>
#include <stdlib.h>
#include "xft.h"

static XftColor *color(Display *dpy,
             uint8_t red, 
             uint8_t green, 
             uint8_t blue) {
  XRenderColor col;
  int scr = DefaultScreen(dpy);
  
  col.red = red * 257;
  col.green = green * 257;
  col.blue = blue * 257;
  col.alpha = ~0;
  
  XftColor *r = malloc(sizeof(XftColor));
  XftColorAllocValue(dpy, DefaultVisual(dpy, scr), DefaultColormap(dpy, scr), &col, r);
  return r;
}

#define X2B(c) ((c >= '0' && c <= '9') ? (c & 0xF) : (((c | 0x20) - 'a') + 10))
static XftColor *xft_hexcol(Display *dpy, const char *str) {
  if(str == NULL) return 0;
  str = (*str == '#') ? str + 1 : str;
  
  if(strlen(str) != 6) {
    return NULL;
  }
  
  uint8_t red = X2B(str[0]);
  red <<= 4;
  red |= X2B(str[1]);

  uint8_t green = X2B(str[2]);
  green <<= 4;
  green |= X2B(str[3]);

  uint8_t blue = X2B(str[4]);
  blue <<= 4;
  blue |= X2B(str[5]);

  return color(dpy, red, green, blue);
}
#undef X2B

 /* Returns an xft_font struct which can be used by other functions in this file.
    Note that xft_font is defined in the local xft.h and is an abstraction on
    top of Xft library functions which are usually camel cased. */

struct xft_font_drw *xft_get_font_drw(Display *dpy,
                                      xcb_window_t win,
                                      const char *name,
                                      const char *color) {
  struct xft_font_drw *font;
  XftResult r;
  XftPattern *pat;
  uint32_t scr = DefaultScreen(dpy);
  font = malloc(sizeof(struct xft_font_drw));
  
  font->col = xft_hexcol(dpy, color);
  if(!font->col) {
    free(font);
    return NULL;
  }
  
  font->dpy = dpy;
  font->win = win;

  pat = XftFontMatch(dpy, DefaultScreen(dpy), XftNameParse(name), &r);

  
  if(r != XftResultMatch) {
    free(font);
    return NULL;
  }
  
  font->drw =  XftDrawCreate(dpy,
                             win,
                             DefaultVisual(dpy, scr),
                             DefaultColormap(dpy, scr));
  
  font->font = XftFontOpenPattern(dpy, pat);
  font->maxheight = font->font->height;

  return font;
};

struct geom xft_text_geom(struct xft_font_drw *font, const char *str, size_t len) {
  struct geom g;
  XGlyphInfo e;
  
  XftTextExtentsUtf8(font->dpy, font->font, (FcChar8 *)str, len, &e);
  g.yOffset = e.y;
  g.height = font->font->ascent + font->font->descent - 1;
  g.width = e.width;
  return g;
}

void xft_draw_text(struct xft_font_drw *font,
                   int x, int y,
                   const char *str,
                   size_t len) {
  
  XftDrawStringUtf8(font->drw, font->col, font->font, x, y + font->font->ascent, (FcChar8 *)str, len);
}

void xft_free(struct xft_font_drw *font) {
  XftColorFree(font->dpy,
               DefaultVisual(font->dpy, DefaultScreen(font->dpy)),
               DefaultColormap(font->dpy, DefaultScreen(font->dpy)),
               font->col);
  XftFontClose(font->dpy, font->font);
  free(font);
}
