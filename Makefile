all:
	cd src;gcc -lXft -lX11 -lX11-xcb -I/usr/include/freetype2 -Wall -Wextra -lxcb -g cfg.c xft.c font.c key.c util.c color.c main.c -o ../xmenu
install:
	install xmenu /usr/bin
