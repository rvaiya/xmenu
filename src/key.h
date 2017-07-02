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


#ifndef _KEYMAP_H_
struct keymap {
  xcb_keysym_t *map;
  size_t sz;
  size_t nkeysyms; /* Number of keysyms key permap. */
  size_t start; /* The code point represented by 0. */
};

/* Returns the keysym associated with the provided keycode. */
xcb_keysym_t lookup_keycode(struct keymap *keymap,
                            xcb_keycode_t code,
                            uint32_t modifiers);
struct keymap *get_keymap(xcb_connection_t *con);
int lookup_key(const char *name, xcb_keysym_t *keysym);
void key_names(char ***lst, size_t *sz);
char *key_name(struct keymap *keymap, xcb_keycode_t keycode, uint16_t mode_mask);
/* Returns 0 if the provided string corresponds to a valid keyname. */
int validate_key_name(const char *name); 
#endif
