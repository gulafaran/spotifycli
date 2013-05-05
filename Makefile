all: spotifycli

spotifycli:
	gcc -O2 -o spotifycli main.c $(shell pkg-config --cflags --libs glib-2.0) $(shell pkg-config --cflags --libs gio-2.0)

clean:
	rm spotifycli
