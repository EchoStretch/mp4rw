.phony: all clean


all:
	+make -C mp4rw
	+make -C example

clean:
	+make -C mp4rw clean
	+make -C example clean
