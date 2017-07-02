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


#include <xcb/xcb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"

#define checkerr(err)                             \
  if(err) {                                       \
    fprintf(stderr,                               \
            "Fatal X11 Error (%s: line %d): %d",	\
            __func__,                             \
            __LINE__,                             \
            err->error_code);                     \
    exit(-1);                                     \
  }                                               

/* Converts a string to the matrix representation consumed by some X11 font related requests. */
static xcb_char2b_t *str2b(const char *str, int len) {
  xcb_char2b_t *res = malloc((len + 1) * sizeof(xcb_char2b_t));
  int i = 0;
  for(;*str;str++)
    res[i++] = (xcb_char2b_t){ 0, *str };
  
  res[i] = (xcb_char2b_t) { 0, 0 };
  return res;
}


struct geom win_geom(xcb_connection_t *con, xcb_window_t win) {
  struct geom dim;
  xcb_get_geometry_cookie_t cookie = xcb_get_geometry(con, win);
  xcb_generic_error_t *err;
  xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(con, cookie, &err);
  
  checkerr(err);
  
  dim.width = reply->width;
  dim.height = reply->height;
  free(reply);
  return dim;
}

/* Obtain the dimensions of a string that would be drawn using the
   given gc.  The target x,y correspond to a font's "baseline", thus
   in order to obtain the proper y coordinate we must add the font's
   ascent from the base line to the original target coordinate, this
   is the 'yoffset'. */
struct geom text_geom(xcb_connection_t *con,
                      xcb_fontable_t font,
                      const char *text,
                      size_t sz) {
  struct geom dim;
  xcb_generic_error_t *err;
  xcb_query_text_extents_reply_t *res =
    xcb_query_text_extents_reply(con, 
                                 xcb_query_text_extents (con, font, sz, str2b(text, strlen(text))),
                                 &err);
  checkerr(err);

  dim.height = res->overall_ascent + res->overall_descent + 1; 
  dim.width = res->overall_width;
  dim.yOffset = res->overall_ascent;
  free(res);
  return dim;
}

int max_text_height(xcb_connection_t *con, xcb_fontable_t font) {
  int height;
  xcb_generic_error_t *err;
  xcb_query_font_reply_t *repl =
    xcb_query_font_reply(con, xcb_query_font(con, font), &err);
  
  checkerr(err);
  
  height = repl->max_bounds.ascent + repl->max_bounds.descent;
  free(repl);
  return height;
}
