runme: main.c xdg-shell.h xdg-shell.c
	gcc main.c xdg-shell.c -l wayland-client -l wayland-cursor -o runme

xdg-shell-unstable-v6.xml:
	curl -O https://cgit.freedesktop.org/wayland/wayland-protocols/plain/unstable/xdg-shell/xdg-shell-unstable-v6.xml

xdg-shell.h: xdg-shell-unstable-v6.xml
	wayland-scanner client-header xdg-shell-unstable-v6.xml xdg-shell.h

xdg-shell.c: xdg-shell-unstable-v6.xml
	wayland-scanner code xdg-shell-unstable-v6.xml xdg-shell.c

.PHONY: clean
clean:
	rm runme xdg-shell.c xdg-shell.h xdg-shell-unstable-v6.xml
