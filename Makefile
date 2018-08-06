EXTRA_CFLAGS := -DMAX_VERBOSE_LEVEL=4

DESTDIR?=/usr
PREFIX?=/local

DEBUG   = -O2
CC      = gcc
INCLUDE = -I$(DESTDIR)$(PREFIX)/include
CFLAGS  = $(DEBUG) -Wall -Wextra $(INCLUDE) -Winline -pipe

LDFLAGS = -L$(DESTDIR)$(PREFIX)/lib
LIBS    = -lwiringPi -lwiringPiDev -lpthread -lrt -lm -lcrypt

ifneq ($V,1)
Q ?= @
endif

SRC     =    main.c serial.c radio.c kiss.c pi_cc_spi.c test.c util.c

OBJ     =       $(SRC:.c=.o)

all: picc1101 

picc1101:   $(OBJ)
	$Q echo [Link]
	$Q $(CC) -o $@ $(OBJ) $(LDFLAGS) $(LIBS)

PHONY: depend
depend:
	makedepend -Y $(SRC)


clean:
	rm -f *.o picc1101
