#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cfg.h"
#include "key.h"
  
#define DEFAULT_FG_COL "#ffffff"
#define DEFAULT_BG_COL "#002b36"
#define DEFAULT_FONT "10x20"
#define DEFAULT_PADDING 0
#define DEFAULT_SPACING 0
#define DEFAULT_WIDTH 1080
#define DEFAULT_KEY_LAST "L"
#define DEFAULT_KEY_MIDDLE "M"
#define DEFAULT_KEY_HOME "H"
#define DEFAULT_KEY_DOWN "j"
#define DEFAULT_KEY_UP "k"
#define DEFAULT_KEY_QUIT "Escape"
#define DEFAULT_KEY_SEL "Return"

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

static xcb_keysym_t xkey(char *name) {
  xcb_keysym_t key;
  
  if(lookup_key(name, &key)) {
    fprintf(stderr, "FATAL: Invalid key: %s\n", name);
    exit(-1);
  }
    
  return key;
}

struct cfg *get_cfg(const char *path) {
  struct cfg *cfg = malloc(sizeof(struct cfg));
  
  cfg->fgcol = DEFAULT_FG_COL;
  cfg->bgcol = DEFAULT_BG_COL;
  cfg->font = DEFAULT_FONT;
  cfg->padding = DEFAULT_PADDING;
  cfg->spacing = DEFAULT_SPACING;
  cfg->width = DEFAULT_WIDTH;
  
  cfg->key_last = xkey(DEFAULT_KEY_LAST);
  cfg->key_middle = xkey(DEFAULT_KEY_MIDDLE);
  cfg->key_home = xkey(DEFAULT_KEY_HOME);
  cfg->key_down = xkey(DEFAULT_KEY_DOWN);
  cfg->key_up = xkey(DEFAULT_KEY_UP);
  cfg->key_quit = xkey(DEFAULT_KEY_QUIT);
  cfg->key_sel = xkey(DEFAULT_KEY_SEL);
  
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
      if(!strcmp(key, "fgcol"))
        cfg->fgcol = strdup(val);
      else if(!strcmp(key, "bgcol"))
        cfg->bgcol = strdup(val);
      else if(!strcmp(key, "font"))
        cfg->font = strdup(val);
      else if(!strcmp(key, "padding"))
        cfg->padding = atoi(val);
      else if(!strcmp(key, "spacing"))
        cfg->spacing = atoi(val);
      else if(!strcmp(key, "width"))
        cfg->width = atoi(val);
      else if(!strcmp(key, "key_last"))
        cfg->key_last = xkey(val);
      else if(!strcmp(key, "key_middle"))
        cfg->key_middle = xkey(val);
      else if(!strcmp(key, "key_home"))
        cfg->key_home = xkey(val);
      else if(!strcmp(key, "key_down"))
        cfg->key_down = xkey(val);
      else if(!strcmp(key, "key_up"))
        cfg->key_up = xkey(val);
      else if(!strcmp(key, "key_quit"))
        cfg->key_quit = xkey(val);
      else if(!strcmp(key, "key_sel"))
        cfg->key_sel = xkey(val);
      else {
        fprintf(stderr, "FATAL: Invalid config entry at: %s: %d\n", path, n);
        exit(-1);
      }
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
//int main(int argc, char **argv) {
//  char path[256];
//  strcpy(path, getenv("HOME"));
//  strcpy(path + strlen(path), "/.xmenurc");
//    
//  struct cfg *cfg = get_cfg(path);
//  
//  if(argc == 2 && !strcmp(argv[1], "-h"))
//    print_help();
//  else if(argc == 2 && !strcmp(argv[1], "-c"))
//    print_cfg(cfg);
//  
//  print_cfg(cfg);
//  return 0;
//}
  
