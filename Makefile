# Generated automatically from Makefile.in by configure.
CC = gcc
VERSION = 0.5
CFLAGS = -g -O2
LIBS = 
FLAGS=$(CFLAGS)  -DSTDC_HEADERS=1 -DHAVE_RTC_H=1 
INSTALL=/bin/install -c
prefix=/usr/local
bindir=$(prefix)${exec_prefix}/bin

all: 
	@cd libUseful-2.0; $(MAKE)
	$(CC) $(FLAGS) $(LIBS) -g -odaytime main.c libUseful-2.0/libUseful-2.0.a

clean:
	@rm -f daytime libUseful-2.0/*.o libUseful-2.0/*.a libUseful-2.0/*.so

install:
	$(INSTALL) daytime $(bindir)
