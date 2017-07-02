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


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cfg.h"
#include "key.h"
#include "color.h"

#define DEFAULT_FG_COL "#FFFFFF"
#define DEFAULT_BG_COL "#002B36"
#define DEFAULT_SEL_FG_COL "#002B36"
#define DEFAULT_SEL_BG_COL "#FFFFFF"
#define DEFAULT_FONT "Monospace:pixelsize=20"
#define DEFAULT_PADDING 0
#define DEFAULT_SPACING 0
#define DEFAULT_WIDTH 800
#define DEFAULT_KEY_LAST "L"
#define DEFAULT_KEY_MIDDLE "M"
#define DEFAULT_KEY_PAGE_UP "C-b"
#define DEFAULT_KEY_PAGE_DOWN "C-f"
#define DEFAULT_KEY_HOME "H"
#define DEFAULT_KEY_DOWN "j"
#define DEFAULT_KEY_UP "k"
#define DEFAULT_KEY_QUIT "Escape"
#define DEFAULT_KEY_SEL "Return"

#define die(msg, ...)                           \
  do {                                          \
    fprintf(stderr, msg, ##__VA_ARGS__);        \
    exit(-1);                                   \
  } while(0)                                   

static int kvp(char *line, char **key, char **val) {
  *key = NULL;
  *val = NULL;
  
  for(;*line != '\0';line++) {
    if(*line != ' ' && !*key)
      *key = line;
    
    if(*line == ':' && !*val) {
      *line++ = '\0';
      for(;isspace(*line);line++);
      *val = line;
    }
  }
  
  if(*(line - 1) == '\n')
    *(line - 1) = '\0';
  
  if(!(*val && *key))
    return -1;
  
  return 0;
}

struct cfg *get_cfg(const char *path, const char *pname) {
  struct cfg *cfg = malloc(sizeof(struct cfg));
  
  cfg->fgcol = DEFAULT_FG_COL;
  cfg->bgcol = DEFAULT_BG_COL;
  cfg->sel_fgcol = DEFAULT_SEL_FG_COL;
  cfg->sel_bgcol = DEFAULT_SEL_BG_COL;
  cfg->font = DEFAULT_FONT;
  cfg->padding = DEFAULT_PADDING;
  cfg->spacing = DEFAULT_SPACING;
  cfg->width = DEFAULT_WIDTH;
  
  cfg->key_last = DEFAULT_KEY_LAST;
  cfg->key_middle = DEFAULT_KEY_MIDDLE;
  cfg->key_home = DEFAULT_KEY_HOME;
  cfg->key_down = DEFAULT_KEY_DOWN;
  cfg->key_up = DEFAULT_KEY_UP;
  cfg->key_page_down = DEFAULT_KEY_PAGE_DOWN;
  cfg->key_page_up = DEFAULT_KEY_PAGE_UP;
  cfg->key_quit = DEFAULT_KEY_QUIT;
  cfg->key_sel = DEFAULT_KEY_SEL;
  
  FILE *fp = fopen(path, "r");
  if(!fp)
    return cfg;

  char *c;
  int n = 0;
  size_t len = 0;
  char *line = NULL;
  
  while(getline(&line, &len, fp) > 0) {
    n++;
    char *key, *val;
  
    if(!kvp(line, &key, &val)) {
      if(!strcmp(key, "font"))
        cfg->font = strdup(val);
      else if(!strcmp(key, "padding"))
        cfg->padding = atoi(val);
      else if(!strcmp(key, "spacing"))
        cfg->spacing = atoi(val);
      else if(!strcmp(key, "width"))
        cfg->width = atoi(val);
      
#define color(name)                                       \
      else if(!strcmp(key, #name)) {                      \
        cfg->name = strdup(val);                          \
        if(validate_color(val))                           \
          die("FATAL: Invalid color for key %s: %s\n", key, val);  \
      }
      
      color(fgcol)
      color(bgcol)
      color(sel_fgcol)
      color(sel_bgcol)
#undef color
      
      
#define key(name)                                                       \
      else if(!strcmp(key, #name)) {                                    \
        cfg->name = strdup(val);                                        \
        if(validate_key_name(val))                                      \
          die("FATAL: Invalid key: %s used for %s, use %s -k to obtain a list of valid keys.\n", val, key, pname); \
      }
                        
      key(key_last)
      key(key_middle)
      key(key_home)
      key(key_down)
      key(key_up)
      key(key_page_up)
      key(key_page_down)
      key(key_quit)
      key(key_sel)
#undef key
      
      else
        die("FATAL: Invalid config entry at (key: %s): %s: %d\n", key, path, n);
      
    } else {
      c = line;
      while(isspace(*c++));
      if(*c != '\0') {
        fprintf(stderr, "FATAL: Invalid config line: %s: %d\n", path, n);
        exit(-1);
      }
      
    }
    
    free(line);
    line = NULL;
    len = 0;
  }
  
  fclose(fp);
  
  return cfg;
}
