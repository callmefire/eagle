CC = gcc
LD = ld
AR = ar
MAKE = make
OBJECTS = tako.o eagle.o seed.o parser.o templates/build_in.o lib/build_in.o
TAKO_OBJS = tako.o seed.o
EAGLE_OBJS = eagle.o seed.o parser.o templates/build_in.o lib/build_in.o

TOPDIR=$(shell pwd)

HEADER_DIR = $(TOPDIR)/include
LIBHDR_DIR = $(TOPDIR)/lib/include

CFLAGS += -Wall -Werror
CFLAGS += -g -pg -DDEBUG
CFLAGS += -I$(HEADER_DIR)
CFLAGS += -I$(LIBHDR_DIR)

export TOPDIR
export CC
export LD
export AR
export MAKE
export CFLAGS

all: tako eagle

tako: $(TAKO_OBJS)
	$(CC) $(CFLAGS) -o tako $(TAKO_OBJS) -lcurl -lpthread -lrt

eagle: $(EAGLE_OBJS)	
	$(CC) $(CFLAGS) -o eagle $(EAGLE_OBJS) -lcurl -lpthread -lrt

.c.o:
	$(CC) $(CFLAGS) -c $<  

templates/build_in.o:
	$(MAKE) -C templates

lib/build_in.o:
	$(MAKE) -C lib

clean:
	@rm -f *~
	@rm -f *.o
	@rm -f tako eagle
	@$(MAKE) clean -C templates
	@$(MAKE) clean -C lib 
