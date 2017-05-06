all:
	cd src;gcc -Wall -Wextra -lxcb -g cfg.c font.c key.c util.c color.c main.c -o xmenu
install:
	install xmenu /usr/bin
