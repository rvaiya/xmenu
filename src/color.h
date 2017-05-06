#ifndef _COLOR_H_
#define _COLOR_H_
#include <stdint.h>
uint32_t hexcol(xcb_connection_t *con, const char *str, int *err);
#endif
