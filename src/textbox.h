#ifndef _TEXTBOX_H_
#define _TEXTBOX_H_

struct textbox {
  Display *dpy;
  xcb_connection_t *con;
  xcb_window_t win;
  xcb_pixmap_t pm;
  xcb_pixmap_t opm;
  xcb_gcontext_t curgc;
  xcb_gcontext_t gc;
  uint32_t x,y;
  /* boolean indicating whether or not to fill the input background with the background color. */
  int fill; 
  int width;
  int height;
  const char *fontname;
  struct xft_font_drw *fdrw;
};

char *textbox_query(struct textbox *ctx);
size_t textbox_height(Display *dpy, const char *font);
struct textbox *textbox_init(Display *dpy, int x, int y, const char *fgcol, const char *bgcol,
                             const char *font, int width, int fill);
#endif
