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
