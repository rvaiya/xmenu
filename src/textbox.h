#ifndef _TEXTBOX_H_
#define _TEXTBOX_H_

char *textbox(Display *dpy,
              int x, int y,
              const char *fgcol,
              const char *bgcol,
              const char *font);

size_t textbox_height(Display *dpy, const char *font);
#endif
