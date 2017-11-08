CFLAGS ?= -g -O2
CFLAGS += -Wall -Wextra -Wconversion
CFLAGS += -D_FILE_OFFSET_BITS=64

.PHONY: all
all: u8strings

.PHONY: install
install: u8strings
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m755 $(<) $(DESTDIR)$(PREFIX)/bin/$(<)
ifeq "$(wildcard .git doc/u8strings.1)" ".git"
	# run "$(MAKE) -C doc" to build the manpage
else
	install -d $(DESTDIR)$(PREFIX)/share/man/man1
	install -m644 doc/u8strings.1 $(DESTDIR)$(PREFIX)/share/man/man1/u8strings.1
endif

.PHONY: test
test: u8strings
	prove -v

.PHONY: clean
clean:
	rm -f u8strings

# vim:ts=4 sts=4 sw=4 noet
