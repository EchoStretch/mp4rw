.phony: all clean

ifndef PS5_PAYLOAD_SDK
    PS5_PAYLOAD_SDK = /opt/ps5-payload-sdk/
endif

include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk

BIN := ../bin/mp4dumper.elf
SRC := $(wildcard *.c)
OBJS = $(SRC:.c=.o)

CFLAGS := -std=c23 -Wall -I../include -march=znver2 -g0 -O3 -flto=full
LDFLAGS := -L../lib -lmp4rw

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^


all: $(BIN)

clean:
	rm -f $(BIN) $(OBJS)

