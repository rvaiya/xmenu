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
const char *key_name(xcb_keysym_t sym);
#endif
