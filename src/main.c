#include <X11/Xlib-xcb.h>
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
#include "xft.h"

#define die(fmt, ...) _die(__func__, __LINE__, fmt, ##__VA_ARGS__)

xcb_screen_t *screen;
xcb_connection_t *con;
Display *dpy;

struct menu_ctx {
  xcb_gcontext_t rect_gc;
  xcb_gcontext_t sel_rect_gc;
  struct xft_font_drw *font;
  struct xft_font_drw *sel_font;
  xcb_window_t win;
  uint32_t padding;
  uint32_t spacing;
  uint32_t sel;
  const char **page;
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

static xcb_gcontext_t rectgc(const char *color) {
  uint32_t col;
  
  col = hexcol(con, color, NULL);
  
  xcb_gcontext_t id = xcb_generate_id(con);
  xcb_create_gc(con, id, screen->root,
                XCB_GC_FOREGROUND |
                XCB_GC_BACKGROUND |
                XCB_GC_FILL_STYLE,
                (uint32_t[]){ col, col, XCB_FILL_STYLE_SOLID });
  return id;
}
  
static struct menu_ctx *menu_ctx(xcb_window_t win,
                                 char *fgcol,
                                 char *bgcol,
                                 char *sel_fgcol,
                                 char *sel_bgcol,
                                 char *font,
                                 int padding,
                                 int spacing) {
  struct menu_ctx *ctx = malloc(sizeof(struct menu_ctx));
  ctx->rect_gc = rectgc(bgcol);
  ctx->font = xft_get_font_drw(dpy, win, font, fgcol);
  
  ctx->sel_rect_gc = rectgc(sel_bgcol);
  ctx->sel_font = xft_get_font_drw(dpy, win, font, sel_fgcol);
  
  ctx->win = win;
  ctx->padding = padding;
  ctx->spacing = spacing;
  ctx->page = NULL;
  return ctx;
}

static void draw_rectangle(xcb_window_t win,
                           int x, int y,
                           int width, int height,
                           xcb_gcontext_t gc) {
  xcb_poly_fill_rectangle(con, win, gc, 1, (xcb_rectangle_t[]){{x, y, width, height}});
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
  xcb_gcontext_t rect_gc;
  struct xft_font_drw *font;
  uint32_t xoffset;
  
  txt = xft_text_geom(ctx->font, item, sz);
  wgeom = win_geom(con, ctx->win);
  height = ctx->font->maxheight + (2 * ctx->padding);
  rect_gc = selected ? ctx->sel_rect_gc : ctx->rect_gc;
  font = selected ? ctx->sel_font : ctx->font;
  yoffset = (height + ctx->spacing) * pos;
  
  xoffset = (alignment == CENTRE) ?
    (wgeom.width - txt.width) / 2 : 0;
  
  draw_rectangle(ctx->win, 0,
                 yoffset,
                 wgeom.width, height, rect_gc);

  xft_draw_text(font,
                xoffset,
                yoffset + ((height - txt.height) / 2),
                item, sz);
}

static void draw_page(const char **page, size_t sz, size_t sel,
                            struct menu_ctx *ctx) {
  size_t i;
  ctx->page = page;

  ctx->sel = sel;
  for (i = 0; i < sz; i++)
    add_item(page[i], strlen(page[i]), i, (i == sel), LEFT, ctx);
  
  xcb_flush(con);
  XFlush(dpy);
}

  
static void menu_update(const char **page, size_t sz, uint32_t sel, struct menu_ctx *ctx) {
  if(!ctx->page)
    ctx->page = page;

  if(page == ctx->page) {
    if(sel == ctx->sel)
        return;
    add_item(page[ctx->sel], strlen(page[ctx->sel]), ctx->sel, 0, LEFT, ctx);
    add_item(page[sel], strlen(page[sel]), sel, 1, LEFT, ctx);
    ctx->sel = sel;
  } else {
    /* If the page has changed redraw everything. */
    draw_page(page, sz, sel, ctx);
  }
  
  xcb_flush(con);
  XFlush(dpy);
}

static void init_con() {
  dpy = XOpenDisplay(NULL);
  con = XGetXCBConnection(dpy);
  XSetEventQueueOwner(dpy, XCBOwnsEventQueue);
  
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

  return id;
}

/* Obtain the dimensions of a menu item that would be produced by the provided context. */
static int menu_entry_height(struct menu_ctx *ctx) {
  return ctx->font->maxheight + (2 * ctx->padding) + ctx->spacing;
}


int menu(const struct cfg *cfg, const char **items, size_t items_sz) {
  struct geom root;
  xcb_window_t win;
  struct menu_ctx *ctx;
  struct keymap *keymap;
  xcb_generic_event_t *ev;
  uint32_t bgcol, winh;
  int sel;
  /* The height occupied by a menu item. */
  int entry_height;
  /* The maximum number of elements that will fit in the window. 
     (Bottleneck is the screen size). */
  int page_sz;
  int opnum = 0;
  const char **page = items;
    
  keymap = get_keymap(con);
 
  root = win_geom(con, screen->root);
  bgcol = hexcol(con, cfg->bgcol, NULL);
  win = create_win(root.width - cfg->width, 0, cfg->width, root.height, bgcol);

  ctx = menu_ctx(win,
                 cfg->fgcol,
                 cfg->bgcol,
                 cfg->sel_fgcol,
                 cfg->sel_bgcol,
                 cfg->font,
                 cfg->padding,
                 cfg->spacing);
 
  entry_height = menu_entry_height(ctx);
  page_sz = entry_height * items_sz > root.height ? root.height / entry_height : items_sz;
  winh = page_sz * entry_height;

  xcb_configure_window(con, win, XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[]){winh});

  xcb_map_window(con, win);
  xcb_set_input_focus(con, XCB_INPUT_FOCUS_PARENT, win, XCB_CURRENT_TIME);
  xcb_flush(con);
  
  sel = 0;
  while((ev = xcb_wait_for_event(con))) {
    const char *keyname;
    
    switch(ev->response_type) {
      case XCB_EXPOSE:
        draw_page(page, page_sz, sel, ctx);
        xcb_flush(con);
        break;
      case XCB_KEY_PRESS:
        keyname = key_name(keymap,
                           ((xcb_key_press_event_t*)ev)->detail,
                           ((xcb_key_press_event_t*)ev)->state);
        if(!keyname) continue;
        
        if(!strcmp(cfg->key_home, keyname)) {
          opnum = opnum ? opnum : 1;
          sel = opnum - 1;
          if(sel >= page_sz)
            sel = page_sz - 1;
          opnum = 0;
        } else if(!strcmp(cfg->key_middle, keyname)) {
          opnum = opnum ? opnum : 1;
          sel = (page_sz - 1) / 2;
          opnum = 0;
        }
        else if(!strcmp(cfg->key_last, keyname)) {
          opnum = opnum ? opnum : 1;
          sel = page_sz - opnum;
          if(sel < 0)
            sel = 0;
          opnum = 0;
        }
        else if(!strcmp(cfg->key_up, keyname)) {
          opnum = opnum ? opnum : 1;
          sel -= opnum;
          if(sel < 0) {
            page -= sel * -1;
            sel = 0;
          }

          if(page < items)
            page = items;
          opnum = 0;
        }
        else if(!strcmp(cfg->key_down, keyname)) {
          opnum = opnum ? opnum : 1;
          sel += opnum;
          if(sel >= page_sz) {
            page += sel - page_sz + 1;
            if(page >= items + items_sz - page_sz)
              page = items + items_sz - page_sz;
            sel = page_sz - 1;
          }
          opnum = 0;
        }
        else if(!strcmp(cfg->key_sel, keyname)) {
          printf("%s\n", page[sel]);
          return 0;
        }
        else if(!strcmp(cfg->key_page_down, keyname)) {
          page += page_sz;
          if(page > items + items_sz - page_sz)
            page = items + items_sz - page_sz;
          sel = 0;
        }
        else if(!strcmp(cfg->key_page_up, keyname)) {
          page -= page_sz;
          if(page < items)
            page = items;
          sel = page_sz - 1;
        }
        else if(!strcmp(cfg->key_quit, keyname))
          exit(-1);
        else if(!strcmp("0", keyname) ||
                !strcmp("1", keyname) ||
                !strcmp("2", keyname) ||
                !strcmp("3", keyname) ||
                !strcmp("4", keyname) ||
                !strcmp("5", keyname) ||
                !strcmp("6", keyname) ||
                !strcmp("7", keyname) ||
                !strcmp("8", keyname) ||
                !strcmp("9", keyname))
          opnum = (opnum * 10) + (keyname[0] & 0xf);
        
        menu_update(page, page_sz, (uint32_t)sel, ctx);
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
length is limited to the width of the screen and truncated if longer. \n\
The configuration parameters described below can \n\
be placed in ~/.xmenurc \n\n\
\
Optional configuration parameters: \n\n\
\
	fgcol: The foreground color. (of the form #xxxxxx) \n\
	bgcol: The background color. (of the form #xxxxxx) \n\
	sel_fgcol: The foreground color of the selected element. (of the form #xxxxxx) \n\
	sel_bgcol: The background color of the selected element. (of the form #xxxxxx) \n\
	font: The xft font pattern to be used. \n\
	padding: The amount of padding \n\
	spacing: The amount of space between elements. \n\
	width: The total width of the selection window. \n\
	key_last: A key which selects the last visible element. \n\
	key_middle: A key which selects the middle element. \n\
	key_home: A key which selects the first element. \n\
	key_down: A key which selects the next element. \n\
	key_page_down: A key which scrolls down one page. \n\
	key_page_up: A key which scrolls up one page. \n\
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
  printf("sel_fgcol: %s\n", c->sel_fgcol);
  printf("sel_bgcol: %s\n", c->sel_bgcol);
  printf("font: %s\n", c->font);
  printf("padding: %d\n", c->padding);
  printf("spacing: %d\n", c->spacing);
  printf("width: %d\n", c->width);
  
  printf("key_last: %s\n", c->key_last);
  printf("key_middle: %s\n", c->key_middle);
  printf("key_home: %s\n", c->key_home);
  printf("key_down: %s\n", c->key_down);
  printf("key_up: %s\n", c->key_up);
  printf("key_page_down: %s\n", c->key_page_down);
  printf("key_page_up: %s\n", c->key_page_up);
  printf("key_quit: %s\n", c->key_quit);
  printf("key_sel: %s\n", c->key_sel);
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
  cfg = get_cfg(cfg_file, argv[0]);
  
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
