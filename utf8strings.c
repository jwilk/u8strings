/* Copyright © 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
 * Copyright © 2014 Jakub Wilk <jwilk@jwilk.net>
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char * progname = "utf8strings";

/* Flexible and Economical UTF-8 Decoder
 * http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
 */

static const unsigned char utf8d[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 00..1f */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 20..3f */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 40..5f */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 60..7f */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, /* 80..9f */
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, /* a0..bf */
    8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* c0..df */
    0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, /* e0..ef */
    0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, /* f0..ff */
    0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, /* s0..s0 */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, /* s1..s2 */
    1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, /* s3..s4 */
    1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, /* s5..s6 */
    1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* s7..s8 */
};

static int extract_strings(const char *path, size_t limit, char radix)
{
    FILE *fp = NULL;
    uintmax_t offset = 0;
    unsigned int state = 0;
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
    if (radix) {
        format[2] = radix;
    } else {
        format[0] = '\0';
    }
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
            break;
        }
        unsigned int ubyte = (unsigned int) byte;
        unsigned int type = utf8d[byte];
        codep = (state != 0) ?
          (ubyte & 0x3FU) | (codep << 6) :
          (0xFFU >> type) & ubyte;
        state = utf8d[256 + state * 16 + type];
        if (state == 0) { /* ACCEPT */
            if (codep == '\t')
                ;
            else if (codep < 0x20)
                state = 1;
            else if ((codep >= 0x7F) && (codep < 0xA0))
                state = 1;
        }
        if (state == 1) { /* REJECT */
            if (nchars >= limit)
                fputc('\n', stdout);
            nbytes = nchars = 0;
            new = 1;
            state = 0;
            continue;
        }
        buffer[nbytes++] = (char) byte;
        if (state == 0) {
            nchars++;
            if (nchars >= limit) {
                if (new) {
                    new = 0;
                    printf(format, offset - nbytes);
                }
                fwrite(buffer, nbytes, 1, stdout);
                nbytes = 0;
                nchars = limit; /* avoid integer overflow */
            }
        }
    }
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
    for (i = optind; i < argc; i++) {
        rc |= extract_strings(argv[i], (size_t) limit, radix);
    }
    if (optind >= argc) {
        rc |= extract_strings(NULL, (size_t) limit, radix);
    }
    return rc;
}

/* vim:set ts=4 sts=4 sw=4 et:*/
