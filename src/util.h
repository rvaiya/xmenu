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
