#ifndef _CFG_H_
#define _CFG_H_
#include <xcb/xcb.h>

struct cfg {
  char *fgcol;
  char *bgcol;
  char *font;
  uint32_t padding;
  uint32_t spacing;
  uint32_t width;
  xcb_keysym_t key_last;
  xcb_keysym_t key_middle;
  xcb_keysym_t key_home;
  xcb_keysym_t key_down;
  xcb_keysym_t key_up;
  xcb_keysym_t key_quit;
  xcb_keysym_t key_sel;
};

struct cfg *get_cfg(const char *path);
#endif
