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
