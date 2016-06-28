PREFIX ?=/usr/local
CC ?= gcc 
CFLAGS = -Wall -I../include

all: tools

tools: ibus0send ibus0dump

install : tools
	install -m 0755 ibus0send $(PREFIX)/bin
	install -m 0755 ibus0dump $(PREFIX)/bin

uninstall : $(PREFIX)/bin/ibus0send $(PREFIX)/bin/ibus0dump
	rm -f $(PREFIX)/bin/midilcm

ibus0send: ibus_send.c
	$(CC) $(CFLAGS) ibus_send.c -o ibus0send

ibus0dump: ibus_dump.c
	$(CC) $(CFLAGS) ibus_dump.c -o ibus0dump

clean :
	rm -f ibus0send ibus0dump
