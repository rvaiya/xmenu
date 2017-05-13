#include <xcb/xcb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "color.h"

#define checkerr(err)                             \
  if(err) {                                       \
    fprintf(stderr,                               \
            "Fatal X11 Error (%s: line %d): %d",	\
            __func__,                             \
            __LINE__,                             \
            err->error_code);                     \
    exit(-1);                                     \
  }                                               

/* Scales an 8 bit number to a 16 bit one (0XFFFF/0XFF = 0x101) */
#define BYTE2SHORT(num) ((0xFF & num) * 0x101)
#define X2B(c) ((c >= '0' && c <= '9') ? (c & 0xF) : (((c | 0x20) - 'a') + 10))

/* Allocates and returns a pixel corresponding to the provided RGB values. */
static uint32_t color(xcb_connection_t *con, uint8_t red, uint8_t green, uint8_t blue) {
  uint32_t pixel;
  xcb_generic_error_t *err;
  xcb_alloc_color_reply_t *repl;
  
  xcb_colormap_t cmap = ((xcb_screen_t *)
                         (xcb_setup_roots_iterator(xcb_get_setup(con)).data))->default_colormap;
  
  repl = xcb_alloc_color_reply(con,
                               xcb_alloc_color(con, cmap,
                                               BYTE2SHORT(red),
                                               BYTE2SHORT(green),
                                               BYTE2SHORT(blue)),
   
                               &err);
  checkerr(err);

  pixel = repl->pixel;
  free(repl);
  return pixel;
}

/* Returns 0 on success. */
static int parse_color(const char *str, uint8_t *red, uint8_t *green, uint8_t *blue) {
  int i;
  if(str == NULL) return -1;
  str = (*str == '#') ? str + 1 : str;
  
  if(strlen(str) != 6)
    return -1;
  
  for (i = 0; i < 6; i++)
    if(!((str[i] >= '0' && str[i] <= '9') || 
         ((str[i] | 0x20) >= 'a' && (str[i] | 0x20) <= 'f')))
      return -1;
  
  *red = X2B(str[0]);
  *red <<= 4;
  *red |= X2B(str[1]);

  *green = X2B(str[2]);
  *green <<= 4;
  *green |= X2B(str[3]);

  *blue = X2B(str[4]);
  *blue <<= 4;
  *blue |= X2B(str[5]);
  
  return 0;
}

/* Returns 0 if the provided string corresponds to a valid hex color string. */
int validate_color(const char *str) {
  uint8_t junk;
  return parse_color(str, &junk, &junk, &junk);
}

/* Consumes a standard hex 256 based  string and converts it into a pixel.
   Populates error if the provided string is not a valid web color. */

uint32_t hexcol(xcb_connection_t *con, const char *str, int *err) {
  uint8_t red, green, blue;
  if(parse_color(str, &red, &green, &blue)) {
    if(err)
      *err = 1;
    return 0;
  }
  
  if(err)
    *err = 0;
  return color(con, red, green, blue);
}
