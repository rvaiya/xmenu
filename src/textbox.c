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


#include <X11/Xlib-xcb.h>
#include "color.h"
#include "xft.h"
#include "key.h"
#include "textbox.h"

/* Returns the number of characters drawn. */
static int update_text(struct textbox *ctx, const char *str) {
  const size_t curw = ctx->fdrw->font->max_advance_width;
  const size_t padding = curw / 2;
  
  struct geom g = xft_text_geom(ctx->fdrw, str, strlen(str));
  
  int len = strlen(str);
  int nchars = (ctx->width - curw) / ctx->fdrw->font->max_advance_width;
  nchars = len < nchars ? len : nchars;
  int curx = len ? nchars * ctx->fdrw->font->max_advance_width + padding : 0;
  
  if(!ctx->fill) {
    xcb_configure_window(ctx->con,
                         ctx->win,
                         XCB_CONFIG_WINDOW_WIDTH,
                         (uint32_t[]){g.width ? g.width + curw + padding : curw});
  } else {
    xcb_clear_area(ctx->con, 1, ctx->pm, 0, 0, ctx->width, ctx->height);
  }
  
  xcb_copy_area(ctx->con, ctx->opm, ctx->pm, ctx->gc, 0, 0, 0, 0, ctx->width, ctx->height);  
  
  xcb_poly_fill_rectangle(ctx->con,
                          ctx->pm,
                          ctx->gc,
                          1,
                          (xcb_rectangle_t[]) {{ 0, 0, ctx->width, ctx->height }});
  
  xcb_poly_fill_rectangle(ctx->con,
                          ctx->pm,
                          ctx->curgc,
                          1,
                          (xcb_rectangle_t[]) {{ curx, 0, curw, ctx->fdrw->maxheight }});
 
  xft_draw_text(ctx->fdrw, 0, 0, str, nchars);
  
  xcb_copy_area(ctx->con, ctx->pm, ctx->win, ctx->gc, 0, 0, 0, 0, ctx->width, ctx->height);
  XFlush(ctx->dpy);
  xcb_flush(ctx->con);
  
  return nchars;
}
  
struct textbox *textbox_init(Display *dpy,
                             xcb_window_t parent,
                             int x, int y,
                             const char *fgcol,
                             const char *bgcol,
                             const char *font,
                             const char *prompt,
                             int width,
                             int fill) {
  
  const char *curcol = fgcol;
  struct textbox *ctx = malloc(sizeof(struct textbox));
  uint32_t pixel;
  xcb_screen_t *screen;
  int err;
  
  ctx->dpy = dpy;
  ctx->con = XGetXCBConnection(dpy);
  ctx->x = x;
  ctx->y = y;
  ctx->fill = fill;
  XSetEventQueueOwner(dpy, XCBOwnsEventQueue);
  
  ctx->pm = xcb_generate_id(ctx->con);
  ctx->opm = xcb_generate_id(ctx->con);
  ctx->curgc = xcb_generate_id(ctx->con);
  ctx->gc = xcb_generate_id(ctx->con);
  ctx->win = xcb_generate_id(ctx->con);

  screen = xcb_setup_roots_iterator(xcb_get_setup(ctx->con)).data;
  
  pixel = hexcol(ctx->con, bgcol, &err);
  if(err)
    return NULL;
  
  xcb_create_window(ctx->con,
                    XCB_COPY_FROM_PARENT,
                    ctx->win,
                    parent,
                    x, y,
                    width, 1,
                    0,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT,
                    screen->root_visual,
                     XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK,
                    (uint32_t[]){1,
                        XCB_EVENT_MASK_EXPOSURE |
                        XCB_EVENT_MASK_KEY_PRESS |
                        XCB_EVENT_MASK_FOCUS_CHANGE});
  
  xcb_create_gc(ctx->con, ctx->gc, parent,
                XCB_GC_FOREGROUND |
                XCB_GC_BACKGROUND |
                XCB_GC_FILL_STYLE,
                (uint32_t[]){ pixel, pixel, XCB_FILL_STYLE_SOLID });


  ctx->fdrw = xft_get_font_drw(dpy, ctx->pm, font, fgcol);
  if(!ctx->fdrw) {
    free(ctx);
    return NULL;
  }
  
  ctx->fontname = font;
  ctx->width = width;
  ctx->height = ctx->fdrw->maxheight;
  
  pixel = hexcol(ctx->con, curcol, &err);
  if(err) return NULL;
  
  xcb_configure_window(ctx->con,
                       ctx->win,
                       XCB_CONFIG_WINDOW_HEIGHT,
                       (uint32_t[]){ctx->height});
  
 /* The main pixmap upon which everything is drawn. */ 
  int depth = xcb_get_geometry_reply(ctx->con, xcb_get_geometry(ctx->con, ctx->win), NULL)->depth;
  
  xcb_create_pixmap(ctx->con,
                    depth,
                    ctx->pm,
                    ctx->win,
                    ctx->width, ctx->height);
  
  xcb_create_pixmap(ctx->con,
                    depth,
                    ctx->opm,
                    ctx->win,
                    ctx->width, ctx->height);

  xcb_map_window(ctx->con, ctx->win);
  xcb_copy_area(ctx->con, ctx->win, ctx->opm, ctx->gc, 0, 0, 0, 0, ctx->width, ctx->height);  
  xcb_unmap_window(ctx->con, ctx->win);
  
  xcb_create_gc(ctx->con, ctx->curgc, ctx->pm,
                XCB_GC_FOREGROUND |
                XCB_GC_BACKGROUND |
                XCB_GC_FILL_STYLE,
                (uint32_t[]){ pixel, pixel, XCB_FILL_STYLE_SOLID });
  
  xcb_flush(ctx->con);
  update_text(ctx, "");
  return ctx;
}

static void hide_window(struct textbox *ctx) {
  xcb_copy_area(ctx->con,
                ctx->opm,
                ctx->win,
                ctx->gc,
                0, 0, 0, 0,
                ctx->width,
                ctx->height);
  
  xcb_unmap_window(ctx->con, ctx->win);
  xcb_flush(ctx->con);
}

static void grab_kbd(xcb_connection_t *con, xcb_window_t win) {
  xcb_grab_keyboard_reply_t *r;
    xcb_generic_error_t *err = NULL;
  r = xcb_grab_keyboard_reply(con, xcb_grab_keyboard(con,
                                                     0,
                                                     win,
                                                     XCB_CURRENT_TIME,
                                                     XCB_GRAB_MODE_SYNC,
                                                     XCB_GRAB_MODE_ASYNC), &err);
                              
  while (r->status != XCB_GRAB_STATUS_SUCCESS) {
    r = xcb_grab_keyboard_reply(con, xcb_grab_keyboard(con,
                                                       0,
                                                       win,
                                                       XCB_CURRENT_TIME,
                                                       XCB_GRAB_MODE_SYNC,
                                                       XCB_GRAB_MODE_ASYNC), NULL);
  }
}


static int evloop(struct textbox *ctx,
                  char *buf,
                  void (*expose_cb)(struct xcb_expose_event_t *ev),
                  int grab_keyboard) {
  struct keymap *keymap;
  xcb_generic_event_t *ev;
  
  xcb_map_window(ctx->con, ctx->win);
  if(grab_keyboard)
    grab_kbd(ctx->con, ctx->win);
  xcb_flush(ctx->con);
  update_text(ctx, "");
  
  keymap = get_keymap(ctx->con);
  *buf = '\0';
  
  while((ev = xcb_wait_for_event(ctx->con))) {
    const char *c;
    const char *keyname;
    int len;
    
    
    switch(ev->response_type) {
      case XCB_KEY_PRESS:
        keyname = key_name(keymap,
                           ((xcb_key_press_event_t*)ev)->detail,
                           ((xcb_key_press_event_t*)ev)->state);
        
        
        if(!keyname)
          continue;
        len = strlen(keyname);
        if(!strcmp(keyname, "BackSpace")) {
          buf[strlen(buf) - 1] = '\0';
        } else if(!strcmp(keyname, "space")) {
          strcat(buf, " ");
        } else if(!strcmp(keyname, "Return")) {
          hide_window(ctx);
          return 0;
        } else if(!strcmp(keyname, "C-u")) {
          *buf = '\0';
        } else if(!strcmp(keyname, "Escape")) {
          hide_window(ctx);
          return -1;
        } else if(len == 2 && keyname[0] == 'F') {
          hide_window(ctx);
          return keyname[1] & 0xF;
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
        
        int printed = update_text(ctx, buf);
        buf[printed] = '\0';
        break;
      case XCB_EXPOSE:
        if(expose_cb)
            expose_cb((struct xcb_expose_event_t*)ev);
        update_text(ctx, buf);
        xcb_flush(ctx->con);
        break;
    }
  }
  return -1;
} 

char *textbox_query(struct textbox *ctx, int *fn, void (*expose_cb)(struct xcb_expose_event_t *ev), int grab_keyboard) {
  int r;
  char *buf = malloc(sizeof(char) * 1024);
  
  if(fn) *fn = 0;
  r = evloop(ctx, buf, expose_cb, grab_keyboard);
  if(fn) *fn = r;
  
  if(r) {
    free(buf);
    return NULL;
  }
  
  return buf;
}
              
size_t textbox_height(Display *dpy, const char *font) {
  size_t height;
  struct xft_font_drw *fdrw = xft_get_font_drw(dpy, DefaultRootWindow(dpy), font, "#000000");
  
  height = fdrw->maxheight;
  xft_free(fdrw);
  return height;
}
