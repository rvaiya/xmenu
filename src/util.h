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


#ifndef _UTIL_H_
#define _UTIL_H_
/* Holds basic dimensional information about different objects. */
struct geom {
  uint32_t width;
  uint32_t height;
  int yOffset;
};


struct geom win_geom(xcb_connection_t *con, xcb_window_t win);
struct geom text_geom(xcb_connection_t *con, xcb_fontable_t font,
                      const char *text, size_t sz);
int max_text_height(xcb_connection_t *con, xcb_fontable_t font);
#endif
