CFLAGS = -m32 -O3 -I ~/lua-5.3.0/src -Wall
LFLAGS = -m32 -L ~/lua-5.3.0/src

%.o: %.c
	gcc $(CFLAGS) -I. -c -o $@ $<

all: luai.exe

luai.exe: main.o image.o path.o realpath.o
	gcc $(LFLAGS) -o $@ $+ -llua

main.o: main.c boot_lua.h

boot_lua.h: boot.lua
	xxd -i $< | sed "s@unsigned@@" > $@

clean:
	rm -f luai.exe main.o image.o path.o realpath.o boot_lua.h
