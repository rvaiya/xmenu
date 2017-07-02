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
#include <stdlib.h>
#include <stdio.h>
#include "font.h"

xcb_font_t get_xls_font(xcb_connection_t *con, char *name, size_t sz) {
  xcb_font_t font = xcb_generate_id(con);
  xcb_void_cookie_t cookie = xcb_open_font_checked(con, font, sz, name);
  xcb_generic_error_t *err = xcb_request_check (con, cookie);
  
  if(err) {
    fprintf(stderr,
            "Fatal X11 Error (%s: line %d): %d\n",
            __func__,
            __LINE__,
            err->error_code);
    exit(-1);
  }
  
  return font;
}
