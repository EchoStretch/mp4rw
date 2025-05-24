.phony: all clean

ifndef PS5_PAYLOAD_SDK
    PS5_PAYLOAD_SDK = /opt/ps5-payload-sdk/
endif

include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk

LIB := ../lib/libmp4rw.a
SRC := $(wildcard *.c)
OBJS = $(SRC:.c=.o)

CFLAGS := -std=c23 -Wall  -I../include -march=znver2 -g0 -O3 -flto=full

all: $(LIB)

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIB): $(OBJS)
	$(AR) -rsc $@ $^

clean:
	rm -f $(LIB) $(OBJS)

