/* Copyright © 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
 * Copyright © 2014-2015 Jakub Wilk <jwilk@jwilk.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char * progname = "u8strings";

/* Flexible and Economical UTF-8 Decoder
 * http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
 */

#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

static const unsigned char utf8d[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* byte = 0x00 .. 0x0F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* byte = 0x10 .. 0x1F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* byte = 0x20 .. 0x2F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* byte = 0x30 .. 0x3F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* byte = 0x40 .. 0x4F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* byte = 0x50 .. 0x5F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* byte = 0x60 .. 0x6F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* byte = 0x70 .. 0x7F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* byte = 0x80 .. 0x8F */
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,  /* byte = 0x90 .. 0x9F */
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  /* byte = 0xA0 .. 0xAF */
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  /* byte = 0xB0 .. 0xBF */
    8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  /* byte = 0xC0 .. 0xCF */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  /* byte = 0xD0 .. 0xDF */
    10, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3,  /* byte = 0xE0 .. 0xEF */
    11, 6, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,  /* byte = 0xFF .. 0xFF */
};

static const unsigned char utf8t[] = {
    0, 1, 2, 3, 5, 8, 7, 1, 1, 1, 4, 6, 1, 1, 1, 1,  /* state = 0 = UTF8_ACCEPT */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* state = 1 = UTF8_REJECT */
    1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1,  /* state = 2 */
    1, 2, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1,  /* state = 3 */
    1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,  /* state = 4 */
    1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,  /* state = 5 */
    1, 1, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 1,  /* state = 6 */
    1, 3, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 1,  /* state = 7 */
    1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* state = 8 */
};

static int is_printable(unsigned int codep)
{
    if (codep == '\t')
        return 1;
    if (codep < 0x20)
        return 0;
    if ((codep >= 0x7F) && (codep < 0xA0))
        return 0;
    return 1;
}

static int extract_strings(const char *path, size_t limit, char radix)
{
    FILE *fp = NULL;
    uintmax_t offset = 0;
    unsigned int state = UTF8_ACCEPT;
    unsigned int codep = 0;
    size_t nbytes = 0;
    size_t nchars = 0;
    int new = 1;
    char *buffer = NULL;
    char format[] = "%jX ";
    assert(limit <= SIZE_MAX / 4); /* no overflow */
    buffer = malloc(limit * 4);
    if (buffer == NULL)
        goto error;
    if (path == NULL)
        fp = stdin;
    else
        fp = fopen(path, "rb");
    if (fp == NULL)
        goto error;
    format[2] = radix;
    offset = 0;
    while (1) {
        if (radix && ++offset == 0) {
            errno = EFBIG;
            goto error;
        }
        int byte = fgetc(fp);
        if (byte == EOF) {
            if (ferror(fp))
                goto error;
            if (feof(fp))
                break;
        }
        unsigned int ubyte = (unsigned int) byte;
        unsigned int type = utf8d[byte];
        unsigned int bitmask = 0xFFU >> type;
        codep = (state == UTF8_ACCEPT) ?
          (ubyte & bitmask) :
          (ubyte & 0x3FU) | (codep << 6);
        state = utf8t[state * 16 + type];
        if ((state == UTF8_ACCEPT) && !is_printable(codep))
            state = UTF8_REJECT;
        if (state == UTF8_REJECT) {
            if (nchars >= limit)
                fputc('\n', stdout);
            nbytes = nchars = 0;
            new = 1;
            /* This wasn't a continuation byte.
             * But maybe it starts a new character.
             * Let's try to resynchronize.
             */
            codep = ubyte & bitmask;
            state = utf8t[type];
            if ((state == UTF8_ACCEPT) && !is_printable(codep))
                continue;
            if (state == UTF8_REJECT) {
                state = UTF8_ACCEPT;
                continue;
            }
        }
        buffer[nbytes++] = (char) byte;
        if (state == UTF8_ACCEPT) {
            nchars++;
            if (nchars >= limit) {
                if (new) {
                    new = 0;
                    if (radix)
                        printf(format, offset - nbytes);
                }
                fwrite(buffer, nbytes, 1, stdout);
                nbytes = 0;
                nchars = limit; /* avoid integer overflow */
            }
        }
    }
    if (nchars >= limit)
        fputc('\n', stdout);
    if (fclose(fp) == EOF)
        goto error;
    free(buffer);
    return 0;
error:
    fprintf(stderr, "%s: %s: %s\n", progname, path, strerror(errno));
    if (fp != NULL)
        fclose(fp); /* ignore errors */
    free(buffer);
    return 1;
}

void flush_stdout(void)
{
    int rc;
    if (ferror(stdout)) {
        fclose(stdout);
        rc = EOF;
        errno = EIO;
    } else
        rc = fclose(stdout);
    if (rc == EOF) {
        fprintf(stderr, "%s: %s\n", progname, strerror(errno));
        exit(1);
    }
}

int main(int argc, char **argv)
{
    int opt;
    long limit = 4;
    char radix = '\0';
    while ((opt = getopt(argc, argv, "an:t:")) != -1)
    switch (opt) {
        case 'a':
            break;
        case 'n': {
            char *endptr;
            errno = 0;
            limit = strtol(optarg, &endptr, 10);
            if (limit <= 0)
                errno = ERANGE;
            else if ((unsigned long)limit > SIZE_MAX / 4)
                errno = ERANGE;
            else if (*endptr != '\0')
                errno = EINVAL;
            if (errno != 0) {
                fprintf(stderr, "%s: invalid minimum string length %s\n", progname, optarg);
                exit(1);
            }
            break;
        }
        case 't':
            printf("%d %d\n", optarg[0], optarg[1]);
            switch (optarg[0]) {
                case 'o':
                case 'x':
                case 'd':
                    if (optarg[1])
                        radix = '\0';
                    else
                        radix = optarg[0];
                    break;
                default:
                    radix = '\0';
            }
            if (!radix) {
                fprintf(stderr, "%s: invalid radix: %s\n", progname, optarg);
                exit(1);
            }
            break;
        default:
            fprintf(stderr, "%s: [-a] [-t FORMAT] [-n LENGTH] [FILE...]\n", progname);
            exit(1);
    }
    int i;
    int rc = 0;
#ifdef __AFL_HAVE_MANUAL_CONTROL
    /* Support for American fuzzy lop deferred forkserver:
     * http://lcamtuf.coredump.cx/afl/
     */
    __AFL_INIT();
#endif
    for (i = optind; i < argc; i++) {
        rc |= extract_strings(argv[i], (size_t) limit, radix);
    }
    if (optind >= argc) {
        rc |= extract_strings(NULL, (size_t) limit, radix);
    }
    flush_stdout();
    return rc;
}

/* vim:set ts=4 sts=4 sw=4 et:*/
