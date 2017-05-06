#include <xcb/xcb.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "color.h"
#include "key.h"
#include "util.h"
#include "font.h"
#include "cfg.h"

#define die(fmt, ...) _die(__func__, __LINE__, fmt, ##__VA_ARGS__)

xcb_screen_t *screen;
xcb_connection_t *con;

struct menu_ctx {
  xcb_gcontext_t sel_rect_gc;
  xcb_gcontext_t sel_text_gc;
  xcb_gcontext_t rect_gc;
  xcb_gcontext_t text_gc;
  xcb_window_t win;
  uint32_t padding;
  uint32_t spacing;
  uint32_t max_text_height;
};

enum alignment {
  LEFT,
  CENTRE,
};

void _die(const char *function, uint32_t line, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  
  fprintf(stderr, "%s: (line %d): ", function, line);
  vfprintf(stderr, fmt, ap);
  
  xcb_disconnect(con);
  va_end(ap);
  exit(-1);
}

static void check_cookie(xcb_void_cookie_t cookie) {
  xcb_generic_error_t *err = xcb_request_check(con, cookie);
  if(err) {
    fprintf(stderr, "Encountered error: %d", err->error_code);
    exit(-1);
  }
}



static xcb_gcontext_t generate_gc(char *fgcolor, char *bgcolor, char *fontname) {
  uint32_t fg, bg;
  xcb_font_t font;
  
  fg = hexcol(con, fgcolor, NULL);
  bg = hexcol(con, bgcolor, NULL);
  font = get_xls_font(con, fontname, strlen(fontname));
  
  xcb_gcontext_t id = xcb_generate_id(con);
  xcb_create_gc(con, id, screen->root,
                XCB_GC_FOREGROUND |
                XCB_GC_BACKGROUND |
                XCB_GC_FILL_STYLE |
                XCB_GC_FONT,
                (uint32_t[]){ fg,
                              bg,
                              XCB_FILL_STYLE_SOLID,
                              font });
  return id;
}
  
static struct menu_ctx *menu_ctx(xcb_window_t win,
                                 char *fgcol,
                                 char *bgcol,
                                 char *font,
                                 int padding,
                                 int spacing) {
  struct menu_ctx *ctx = malloc(sizeof(struct menu_ctx));
  ctx->rect_gc = generate_gc(bgcol, bgcol, font);
  ctx->text_gc = generate_gc(fgcol, bgcol, font);
  ctx->sel_rect_gc = generate_gc(fgcol, fgcol, font);
  ctx->sel_text_gc = generate_gc(bgcol, fgcol, font);
  ctx->max_text_height = max_text_height(con, ctx->text_gc);
  ctx->win = win;
  ctx->padding = padding;
  ctx->spacing = spacing;
  return ctx;
}

static void draw_rectangle(xcb_window_t win,
                           int x, int y,
                           int width, int height,
                           xcb_gcontext_t gc) {
  check_cookie(xcb_poly_fill_rectangle(con, win, gc, 1,
                                       (xcb_rectangle_t[]){{x, y, width, height}}));
}

/* Adds a menu item to the provided window. */
static void draw_text(xcb_window_t win,
                      const char *text,
                      int sz,
                      int x, int y,
                      xcb_gcontext_t gc) {
  struct geom g = text_geom(con, gc, text, sz);
  check_cookie(xcb_image_text_8(con, sz, win, gc, x, g.yoffset + y, text));
}

static void add_item(const char *item,
                     size_t sz,
                     int pos,
                     int selected,
                     enum alignment alignment,
                     struct menu_ctx *ctx) {
  int height;
  int yoffset;
  struct geom txt;
  struct geom wgeom;
  xcb_gcontext_t rect_gc, text_gc;
  uint32_t xoffset;
  
  txt = text_geom(con, ctx->text_gc, item, sz);
  wgeom = win_geom(con, ctx->win);
  height = ctx->max_text_height + (2 * ctx->padding);
  rect_gc = selected ? ctx->sel_rect_gc : ctx->rect_gc;
  text_gc = selected ? ctx->sel_text_gc : ctx->text_gc;
  yoffset = (height + ctx->spacing) * pos;
  
  if(alignment == CENTRE)
    xoffset = (wgeom.width - txt.width) / 2;
  
  draw_rectangle(ctx->win, 0,
                 yoffset,
                 wgeom.width, height, rect_gc);

  draw_text(ctx->win,
            item, sz,
            xoffset,
            yoffset + ((height - txt.height) / 2),
            text_gc);

  xcb_flush(con);
}
  
static void draw_menu_items(const char **items, int sz, int sel,
                            struct menu_ctx *ctx) {
  int i;
  
  /* Synchronous, but meh.. */
  for (i = 0; i < sz; i++)
    add_item(items[i], strlen(items[i]), i, (i == sel), LEFT, ctx);
}

static void init_con() {
  con = xcb_connect(NULL, NULL);
  const xcb_setup_t *setup = xcb_get_setup(con);
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
  screen = iter.data;
}

static xcb_window_t create_win(int x, int y, int width, int height, uint32_t color) {
  xcb_window_t id = xcb_generate_id(con);
  xcb_create_window(con,
                    XCB_COPY_FROM_PARENT,
                    id,
                    screen->root,
                    x, y,
                    width, height,
                    0,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT,
                    screen->root_visual,
                    XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK,
                    (uint32_t[]){color, 1,
                        XCB_EVENT_MASK_EXPOSURE |
                        XCB_EVENT_MASK_KEY_PRESS |
                        XCB_EVENT_MASK_FOCUS_CHANGE});

  xcb_map_window(con, id);
  xcb_flush(con);
  return id;
}

/* Obtain the dimensions of a menu item that would be produced by the provided context. */
static int menu_entry_height(struct menu_ctx *ctx) {
  int height = 0;
  
  height = max_text_height(con, ctx->text_gc);
  return height + (2 * ctx->padding) + ctx->spacing;
}

static xcb_keysym_t xkey(char *name) {
  xcb_keysym_t key;
  if(lookup_key(name, &key))
    die("Lookup of key %s failed\n", name);
  return key;
}

int menu(const struct cfg *cfg, const char **items, size_t sz) {
  struct geom root;
  xcb_window_t win;
  struct menu_ctx *ctx;
  struct keymap *keymap;
  xcb_generic_event_t *ev;
  uint32_t bgcol;
  int sel;
  /* The height occupied by a menu item. */
  int entry_height;
  /* The maximum number of elements that will fit in the window. 
     (Bottleneck is the screen size). */
  int sel_sz;
  int opnum = 0;
    
  keymap = get_keymap(con);
 
  root = win_geom(con, screen->root);
  bgcol = hexcol(con, cfg->bgcol, NULL);
  win = create_win(root.width - cfg->width, 0, cfg->width, root.height, bgcol);
  ctx = menu_ctx(win, cfg->fgcol, cfg->bgcol, cfg->font, 10, 0);
 
  entry_height = menu_entry_height(ctx);
  
  xcb_configure_window(con, win, XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[]){entry_height * sz});
  xcb_set_input_focus(con, XCB_INPUT_FOCUS_PARENT, win, XCB_CURRENT_TIME);
  
  sel_sz = (root.height / entry_height) < sz ? (root.height / entry_height) : sz;
  
  sel = 0;
  while((ev = xcb_wait_for_event(con))) {
    xcb_keysym_t keysym;
    
    switch(ev->response_type) {
      case XCB_EXPOSE:
        draw_menu_items(items, sz, sel, ctx);
        xcb_flush(con);
        break;
      case XCB_KEY_PRESS:
        keysym = lookup_keycode(keymap,
                                ((xcb_key_press_event_t*)ev)->detail,
                                ((xcb_key_press_event_t*)ev)->state);
        if(keysym == cfg->key_home) {
          opnum = opnum ? opnum : 1;
          sel = opnum - 1;
          if(sel >= sel_sz)
            sel = sel_sz - 1;
          opnum = 0;
        } else if(keysym == cfg->key_middle) {
          opnum = opnum ? opnum : 1;
          sel = (sel_sz - 1) / 2;
          opnum = 0;
        }
        else if(keysym == cfg->key_last) {
          opnum = opnum ? opnum : 1;
          sel = sel_sz - opnum;
          if(sel < 0)
            sel = 0;
          opnum = 0;
        }
        else if(keysym == cfg->key_down) {
          opnum = opnum ? opnum : 1;
          sel = (sel + opnum) > (sel_sz - 1) ? (sel_sz - 1) : (sel + opnum);
          opnum = 0;
        }
        else if(keysym == cfg->key_up) {
          opnum = opnum ? opnum : 1;
          sel -= opnum;
          if(sel < 0)
            sel = 0;
          opnum = 0;
        } else if(keysym == cfg->key_sel) {
          printf("%s\n", items[sel]);
          return 0;
        }
        else if(keysym == cfg->key_quit)
          exit(-1);
        else if(keysym == xkey("0") ||
                keysym == xkey("1") ||
                keysym == xkey("2") ||
                keysym == xkey("3") ||
                keysym == xkey("4") ||
                keysym == xkey("5") ||
                keysym == xkey("6") ||
                keysym == xkey("7") ||
                keysym == xkey("8") ||
                keysym == xkey("9"))
          opnum = (opnum * 10) + (key_name(keysym)[0] & 0xf);
        
        draw_menu_items(items, sz, sel, ctx);
        xcb_flush(con);
        break;
      case XCB_FOCUS_OUT:
        xcb_set_input_focus(con, XCB_INPUT_FOCUS_PARENT, win, XCB_CURRENT_TIME);
        xcb_flush(con);
        break;
    }
    free(ev);
  }
  
  pause();
  xcb_disconnect(con);
  return 0;

  
}

void print_keynames() {
  size_t i;
  char **lst;
  size_t sz;
  key_names(&lst, &sz);
  for (i = 0; i < sz; i++)
    printf("%s\n", lst[i]);
  
  exit(1);
}

void print_help() {
printf("\
A tiny X11 program which displays a menu of items corresponding to \n\
input lines and prints the selected one to STDOUT. Items are drawn \n\
from the provided file or STDIN if no arguments are provided.  Item \n\
length is limited to the width of the screen and truncated if longer, \n\
the number of selectable items is similarly limited by screen height \n\
(i.e no scrolling). The configuration parameters described below can \n\
be placed in ~/.xmenurc \n\n\
\
Optional configuration parameters: \n\n\
\
	fgcol: The foreground color. (of the form #xxxxxx) \n\
	bgcol: The background color. (of the form #xxxxxx) \n\
	font: The core X11 font used to print characters, see xlsfonts(1). (XFT not supported)  \n\
	padding: The amount of padding \n\
	spacing: The amount of space between elements. \n\
	width: The total width of the selection window. \n\
	key_last: A key which selects the last visible element. \n\
	key_middle: A key which selects the middle element. \n\
	key_home: A key which selects the first element. \n\
	key_down: A key which selects the next element. \n\
	key_up: A key which selects the previous element. \n\
	key_quit: A key which exists the dialogue without printing any items. \n\
	key_sel: A key which closes the menu and prints the selected item to STDOUT. \n\n\
\
Arguments: \n\n\
\
	-h: Prints this help message. \n\
	-c: Prints the merged configuration parameters. \n\
	-k: Prints a list of valid key names that can be used in the config file. \n\n\
");
 exit(-1);
}

void print_cfg(struct cfg *c) {
  printf("fgcol: %s\n", c->fgcol);
  printf("bgcol: %s\n", c->bgcol);
  printf("font: %s\n", c->font);
  printf("padding: %d\n", c->padding);
  printf("spacing: %d\n", c->spacing);
  printf("width: %d\n", c->width);
  
  printf("key_last: %s\n", key_name(c->key_last));
  printf("key_middle: %s\n", key_name(c->key_middle));
  printf("key_home: %s\n", key_name(c->key_home));
  printf("key_down: %s\n", key_name(c->key_down));
  printf("key_up: %s\n", key_name(c->key_up));
  printf("key_quit: %s\n", key_name(c->key_quit));
  printf("key_sel: %s\n", key_name(c->key_sel));
  exit(1);
} 


char **read_items(size_t *sz, FILE *fp) {
  size_t len;
  size_t c = 1;
  size_t n = 0;
  char *line;
  char **lines = malloc(c * sizeof(char*));
  
  line = NULL;len = 0;
  while(getline(&line, &len, fp) > -1) {
    if(++n > c)
      lines = realloc(lines, sizeof(char*) * (c *= 2));
    
    len = strlen(line);
    if(len)
        line[len - 1] = '\0';
    lines[n - 1] = line;
    line = NULL;len = 0;
  }
  
  *sz = n;
  return lines;
}

int main(int argc, char **argv) {
  FILE *file;
  char cfg_file[256];
  struct cfg *cfg;
  size_t nitems;
  const char **items;
  
  init_con();
  
  strncpy(cfg_file, getenv("HOME"), 240);
  strcpy(cfg_file + strlen(cfg_file), "/.xmenurc");
  cfg = get_cfg(cfg_file);
  
  if(argc == 2 && !strcmp(argv[1], "-h"))
    print_help();
  else if(argc == 2 && !strcmp(argv[1], "-c"))
    print_cfg(cfg);
  else if(argc == 2 && !strcmp(argv[1], "-k"))
    print_keynames();
  else if(argc == 2) {
    file = fopen(argv[1], "r");
    if(!file) {
      fprintf(stderr, "ERROR: %s is not a valid input file.\n", argv[1]);
      print_help();
    }
  }
  else if(argc == 1)
    file = stdin;
  else {
    fprintf(stderr, "%s [ -h | -c | -k | <file>]", argv[0]);
    exit(-1);
  }

  items = (const char**)read_items(&nitems, file);
  menu(cfg,
       items,
       nitems);
  
  return 0;
}
