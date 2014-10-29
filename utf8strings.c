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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Flexible and Economical UTF-8 Decoder
 * http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
 */

static const uint8_t utf8d[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
    8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
    0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
    0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
    0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
    1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
    1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
    1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};

int extract_strings(const char *path, size_t limit)
{
    uint32_t state = 0;
    uint32_t codep = 0;
    size_t nbytes = 0;
    size_t nchars = 0;
    char *buffer = NULL;
    if (limit > SIZE_MAX / 4) {
        errno = ENOMEM;
        goto error;
    }
    buffer = malloc(limit * 4);
    if (buffer == NULL)
        goto error;
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
        goto error;
    while (1) {
        int byte = fgetc(fp);
        if (byte == EOF) {
            if (ferror(fp))
                goto error;
            break;
        }
        uint32_t type = utf8d[byte];
        codep = (state != 0) ?
          (byte & 0x3FU) | (codep << 6) :
          (0xFF >> type) & (byte);
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
            state = 0;
            continue;
        }
        buffer[nbytes++] = byte;
        if (state == 0) {
            nchars++;
            if (nchars >= limit) {
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
    fprintf(stderr, "utf8strings: %s: %s\n", path, strerror(errno));
    if (buffer != NULL)
        free(buffer);
    return 1;
}

int main(int argc, char **argv)
{
    int i;
    int rc = 0;
    for (i = 1; i < argc; i++) {
        rc |= extract_strings(argv[i], 4);
    }
    return rc;
}

/* vim:set ts=4 sts=4 sw=4 et:*/
