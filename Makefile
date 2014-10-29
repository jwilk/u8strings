CFLAGS ?= -g -O2
CFLAGS += -Wall 
CFLAGS += $(shell getconf LFS_CFLAGS)

.PHONY: all
all: utf8strings

.PHONY: clean
clean:
	rm -f utf8strings

# vim:ts=4 sts=4 sw=4 noet
