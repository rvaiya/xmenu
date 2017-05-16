#include <X11/Xlib-xcb.h>
#include "color.h"
#include "xft.h"
#include "key.h"

struct ctx {
  Display *dpy;
  xcb_connection_t *con;
  xcb_window_t win;
  xcb_gcontext_t curgc;
  uint32_t x,y, maxw;
  struct xft_font_drw *drw;
};

static void update_text(struct ctx *ctx, const char *str) {
  const size_t curw = ctx->drw->font->max_advance_width;
  const size_t padding = curw / 2;
  struct geom g = xft_text_geom(ctx->drw, str, strlen(str));
  xcb_configure_window(ctx->con, ctx->win, XCB_CONFIG_WINDOW_WIDTH, (uint32_t[]){g.width + curw + padding});
  xcb_poly_fill_rectangle(ctx->con,
                          ctx->win,
                          ctx->curgc,
                          1,
                          (xcb_rectangle_t[]) {{ g.width + padding, 0, curw, ctx->drw->maxheight }});
  
  xft_draw_text(ctx->drw, 0, (ctx->drw->maxheight - g.height) / 2, str, strlen(str));
  
  xcb_flush(ctx->con);
  XFlush(ctx->dpy); 
}
  
struct ctx *init(Display *dpy,
                 const char *fgcol,
                 const char *bgcol,
                 const char *curcol,
                 const char *font,
                 int x,
                 int y,
                 int max_width) {
  
  struct ctx *ctx = malloc(sizeof(struct ctx));
  uint32_t pixel;
  xcb_screen_t *screen;
  int err;
  
  ctx->dpy = dpy;
  ctx->con = XGetXCBConnection(dpy);
  ctx->x = x;
  ctx->y = y;
  ctx->maxw = max_width;
  XSetEventQueueOwner(dpy, XCBOwnsEventQueue);

  screen = xcb_setup_roots_iterator(xcb_get_setup(ctx->con)).data;
  
  
  pixel = hexcol(ctx->con, curcol, &err);
  if(err)
    return NULL;
  ctx->curgc = xcb_generate_id(ctx->con);
  xcb_create_gc(ctx->con, ctx->curgc, screen->root,
                XCB_GC_FOREGROUND |
                XCB_GC_BACKGROUND |
                XCB_GC_FILL_STYLE,
                (uint32_t[]){ pixel, pixel, XCB_FILL_STYLE_SOLID });
  
  
  pixel = hexcol(ctx->con, bgcol, &err);
  if(err)
    return NULL;
  ctx->win = xcb_generate_id(ctx->con);
  xcb_create_window(ctx->con,
                    XCB_COPY_FROM_PARENT,
                    ctx->win,
                    screen->root,
                    x, y,
                    1, 1,
                    0,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT,
                    screen->root_visual,
                    XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK,
                    (uint32_t[]){pixel, 1,
                        XCB_EVENT_MASK_EXPOSURE |
                        XCB_EVENT_MASK_KEY_PRESS |
                        XCB_EVENT_MASK_FOCUS_CHANGE});

  ctx->drw = xft_get_font_drw(dpy, ctx->win, font, fgcol);
  
  if(!ctx->drw) {
    free(ctx);
    return NULL;
  }
  
  xcb_configure_window(ctx->con,
                       ctx->win,
                       XCB_CONFIG_WINDOW_HEIGHT,
                       (uint32_t[]){ctx->drw->maxheight});

  
  xcb_map_window(ctx->con, ctx->win);
  
  xcb_flush(ctx->con);
  XFlush(dpy); 
  
  update_text(ctx, "");
  return ctx;
}

static int evloop(struct ctx *ctx, char *buf) {
  struct keymap *keymap;
  xcb_generic_event_t *ev;
  
  xcb_set_input_focus(ctx->con, XCB_INPUT_FOCUS_PARENT, ctx->win, XCB_CURRENT_TIME);
  keymap = get_keymap(ctx->con);
  *buf = '\0';
  
  while((ev = xcb_wait_for_event(ctx->con))) {
    const char *c;
    const char *keyname;
    
    
    switch(ev->response_type) {
      case XCB_KEY_PRESS:
        keyname = key_name(keymap,
                           ((xcb_key_press_event_t*)ev)->detail,
                           ((xcb_key_press_event_t*)ev)->state);
        
        
        if(!keyname)
          continue;
        if(!strcmp(keyname, "BackSpace")) {
          buf[strlen(buf) - 1] = '\0';
        } else if(!strcmp(keyname, "space")) {
          strcat(buf, " ");
        } else if(!strcmp(keyname, "Return")) {
          xcb_destroy_window(ctx->con, ctx->win);
          return 0;
        } else if(!strcmp(keyname, "C-u")) {
          *buf = '\0';
        } else if(!strcmp(keyname, "Escape")) {
          xcb_destroy_window(ctx->con, ctx->win);
          return -1;
        } else if(strlen(keyname) == 1) {
          c = key_name(keymap,
                       ((xcb_key_press_event_t*)ev)->detail,
                       ((xcb_key_press_event_t*)ev)->state);
          if(c)
            strcat(buf, c);
        } else {
          xcb_keysym_t sym;
          int len;
          sym = lookup_keycode(keymap,
                       ((xcb_key_press_event_t*)ev)->detail,
                       ((xcb_key_press_event_t*)ev)->state);
          if(sym < 256) {
            len = strlen(buf);
            buf[len] = sym;
            buf[len + 1] = '\0';
          }
        }
        
        update_text(ctx, buf);
    }
  }
  return -1;
} 

char *textbox(Display *dpy,
              int x, int y,
              const char *fgcol,
              const char *bgcol,
              const char *font) { 
  char *buf = malloc(sizeof(char) * 256);
  
  struct ctx *ctx = init(dpy, fgcol, bgcol, fgcol, font, x, y, 300);
  
  if(evloop(ctx, buf)) {
    free(buf);
    return NULL;
  }
  
  return buf;
}

size_t textbox_height(Display *dpy, const char *font) {
  size_t height;
  struct xft_font_drw *drw = xft_get_font_drw(dpy, DefaultRootWindow(dpy), font, "#000000");
  
  height = drw->maxheight;
  xft_free(drw);
  return height;
}
