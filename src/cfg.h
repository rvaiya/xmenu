#ifndef _CFG_H_
#define _CFG_H_
#include <xcb/xcb.h>

struct cfg {
  char *fgcol;
  char *bgcol;
  char *sel_bgcol;
  char *sel_fgcol;
  char *font;
  uint32_t padding;
  uint32_t spacing;
  uint32_t width;
  const char *key_last;
  const char *key_middle;
  const char *key_home;
  const char *key_down;
  const char *key_up;
  const char *key_page_up;
  const char *key_page_down;
  const char *key_quit;
  const char *key_sel;
};

struct cfg *get_cfg(const char *path, const char *pname);
#endif
