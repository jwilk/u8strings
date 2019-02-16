# Copyright © 2014-2018 Jakub Wilk <jwilk@jwilk.net>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the “Software”), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

CFLAGS ?= -g -O2
CFLAGS += -Wall -Wextra -Wconversion
CFLAGS += -D_FILE_OFFSET_BITS=64

PREFIX = /usr/local
DESTDIR =

.PHONY: all
all: u8strings

.PHONY: install
install: u8strings
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m755 $(<) $(DESTDIR)$(PREFIX)/bin/$(<)
ifeq "$(wildcard doc/u8strings.1)" ""
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

.error = GNU make is required

# vim:ts=4 sts=4 sw=4 noet
