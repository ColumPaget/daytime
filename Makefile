CC = gcc
CFLAGS = -g -O2
LIBS = -lc  -lm
FLAGS=$(CFLAGS) -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DSTDC_HEADERS=1 -D_FILE_OFFSET_BITS=64 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_RTC_H=1 -DHAVE_LIBC=1 -DHAVE_STIME=1
INSTALL=/bin/install -c
prefix=/usr/local
bindir=$(prefix)${exec_prefix}/bin
mandir=${prefix}/share/man/man1
OBJ=common.o sysclock.o sntp.o command-line-args.o

all: $(OBJ)
	@cd libUseful; $(MAKE)
	$(CC) $(FLAGS) $(LIBS) -g -odaytime $(OBJ) main.c libUseful/libUseful.a

common.o: common.h common.c
	$(CC) $(FLAGS) -c common.c

sysclock.o: sysclock.h sysclock.c common.h
	$(CC) $(FLAGS) -c sysclock.c

command-line-args.o: command-line-args.h command-line-args.c common.h
	$(CC) $(FLAGS) -c command-line-args.c

sntp.o: sntp.c sntp.h common.h
	$(CC) $(FLAGS) -c sntp.c

clean:
	@rm -f daytime *.o libUseful/*.o libUseful/*.a libUseful/*.so config.cache config.status config.log */config.cache */config.status */config.log

install:
	$(INSTALL) daytime $(bindir)
	$(INSTALL) daytime.1 $(mandir)

test:
	-echo "no tests"
