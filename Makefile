CC = gcc
LD = ld
AR = ar
MAKE = make
OBJECTS = tako.o eagle.o seed.o parser.o templates/build_in.o lib/build_in.o
TAKO_OBJS = tako.o seed.o
EAGLE_OBJS = eagle.o seed.o parser.o templates/build_in.o lib/build_in.o
SUBDIRS = lib
SUBDIRS += templates

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
	@$(CC) $(CFLAGS) -o tako $(TAKO_OBJS) -lcurl -lpthread -lrt
	@echo "CC $@" 
eagle: build $(EAGLE_OBJS)	
	@$(CC) $(CFLAGS) -o eagle $(EAGLE_OBJS) -lcurl -lpthread -lrt
	@echo "CC $@" 

.c.o:
	@$(CC) $(CFLAGS) -c $<  
	@echo "CC $@" 

.PHONY: build
build:
	@$(foreach dir,$(SUBDIRS),$(MAKE) -s -C $(dir);)

clean:
	@rm -f *~
	@rm -f *.o
	@rm -f tako eagle
	@$(foreach dir,$(SUBDIRS),$(MAKE) clean -s -C $(dir);)
