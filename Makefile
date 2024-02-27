CC = gcc
CFLAGS = -g -O2
LIBS = -lUseful-5 -lz -lz -lssl -lcrypto  -lm
FLAGS=$(CFLAGS) -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DHAVE_STDIO_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_STRINGS_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_UNISTD_H=1 -DSTDC_HEADERS=1 -DHAVE_RTC_H=1 -DHAVE_STIME=1 -DHAVE_LIBCRYPTO=1 -DHAVE_LIBSSL=1 -DHAVE_LIBZ=1 -DHAVE_LIBZ=1 -DHAVE_LIBUSEFUL_5_LIBUSEFUL_H=1 -DHAVE_LIBUSEFUL_5=1
INSTALL=/usr/bin/install -c
prefix=/usr/local
datarootdir=${prefix}/share
bindir=$(prefix)${exec_prefix}/bin
mandir=${datarootdir}/man/man1
OBJ=common.o sysclock.o sntp.o command-line-args.o 

all: $(OBJ)
	$(CC) $(FLAGS) -g -odaytime $(OBJ) main.c  $(LIBS) 

:
	$(MAKE) -C libUseful

common.o: common.h common.c
	$(CC) $(FLAGS) -c common.c

sysclock.o: sysclock.h sysclock.c common.h
	$(CC) $(FLAGS) -c sysclock.c

command-line-args.o: command-line-args.h command-line-args.c common.h
	$(CC) $(FLAGS) -c command-line-args.c

sntp.o: sntp.c sntp.h common.h
	$(CC) $(FLAGS) -c sntp.c

clean:
	@rm -f daytime *.o libUseful/*.o libUseful/*.a libUseful/*.so config.cache config.status config.log */config.cache */config.status */config.log *.orig

install:
	$(INSTALL) daytime $(DESTDIR)$(bindir)
	$(INSTALL) daytime.1 $(DESTDIR)$(mandir)

test:
	-echo "no tests"
