#include <stdio.h>
#include <string.h>
#include <ctype.h>

/** hexadecimal lookup table */ 
static char hex[] = "0123456789ABCDEF";

/** URL unsafe printable characters */ 
static char urlunsafe[] = " \"#%&+:;<=>?@[\\]^`{|}";

/** UTF7 modified base64*/
static char base64chars[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+,";
#define UNDEFINED 64

/** UTF16 definitions */
#define UTF16MASK       0x03FFUL
#define UTF16SHIFT      10
#define UTF16BASE       0x10000UL
#define UTF16HIGHSTART  0xD800UL
#define UTF16HIGHEND    0xDBFFUL
#define UTF16LOSTART    0xDC00UL
#define UTF16LOEND      0xDFFFUL

/***********************************************************
 * Convert IMAP4 modified UTF7 to UTF8
 *	
 ***********************************************************/
void IMAP4UTF72UTF8(char *dst, char *src)
{
    unsigned char c, i, bitcount;
    unsigned long ucs4, utf16, bitbuf;
    unsigned char base64[256], utf8[6];

    /* initialize modified base64 decoding table */
    memset(base64, UNDEFINED, sizeof (base64));
    for (i = 0; i < sizeof (base64chars); ++i) {
        base64[(short)base64chars[i]] = i;
    }

    /* loop until end of string */
    while (*src != '\0') {
        c = *src++;
        /* deal with literal characters and &- */
        if (c != '&' || *src == '-') {
            if (c < ' ' || c > '~' || strchr(urlunsafe, c) != NULL) {
                /* hex encode if necessary */
                dst[0] = '%';
                dst[1] = hex[c >> 4];
                dst[2] = hex[c & 0x0f];
                dst += 3;
            } else {
                /* encode literally */
                *dst++ = c;
            }
            /* skip over the '-' if this is an &- sequence */
            if (c == '&') ++src;
        } else {
        /* convert modified UTF-7 -> UTF-16 -> UCS-4 -> UTF-8 -> HEX */
            bitbuf = 0;
            bitcount = 0;
            ucs4 = 0;
            while ((c = base64[(unsigned char) *src]) != UNDEFINED) {
                ++src;
                bitbuf = (bitbuf << 6) | c;
                bitcount += 6;
                /* enough bits for a UTF-16 character? */
                if (bitcount >= 16) {
                    bitcount -= 16;
                    utf16 = (bitcount ? bitbuf >> bitcount
                             : bitbuf) & 0xffff;
                    /* convert UTF16 to UCS4 */
                    if
                    (utf16 >= UTF16HIGHSTART && utf16 <= UTF16HIGHEND) {
                        ucs4 = (utf16 - UTF16HIGHSTART) << UTF16SHIFT;
                        continue;
                    } else if
                    (utf16 >= UTF16LOSTART && utf16 <= UTF16LOEND) {
                        ucs4 += utf16 - UTF16LOSTART + UTF16BASE;
                    } else {
                        ucs4 = utf16;
                    }
                    /* convert UTF-16 range of UCS4 to UTF-8 */
                    if (ucs4 <= 0x7fUL) {
                        utf8[0] = ucs4;
                        i = 1;
                    } else if (ucs4 <= 0x7ffUL) {
                        utf8[0] = 0xc0 | (ucs4 >> 6);
                        utf8[1] = 0x80 | (ucs4 & 0x3f);
                        i = 2;
                    } else if (ucs4 <= 0xffffUL) {
                        utf8[0] = 0xe0 | (ucs4 >> 12);
                        utf8[1] = 0x80 | ((ucs4 >> 6) & 0x3f);
                        utf8[2] = 0x80 | (ucs4 & 0x3f);
                        i = 3;
                    } else {
                        utf8[0] = 0xf0 | (ucs4 >> 18);
                        utf8[1] = 0x80 | ((ucs4 >> 12) & 0x3f);
                        utf8[2] = 0x80 | ((ucs4 >> 6) & 0x3f);
                        utf8[3] = 0x80 | (ucs4 & 0x3f);
                        i = 4;
                    }

                    for (c = 0; c < i; ++c)
                        dst[c] = utf8[c];
                    dst+=i;
                }
            }
            /* skip over trailing '-' in modified UTF-7 encoding */
            if (*src == '-') ++src;
        }
    }
    /* terminate destination string */
    *dst = '\0';
}

/***********************************************************
 * Convert UTF8 to IMAP4 modified UTF7
 ***********************************************************/
void UTF8IMAP4UTF7(char *dst, char *src)
{
   unsigned int utf8pos=0, utf8total, i, c, utf7mode, bitstogo, utf16flag;
   unsigned long ucs4=0, bitbuf=0;
   unsigned char hextab[256];

    /* initialize hex lookup table */
    memset(hextab, 0, sizeof (hextab));
    for (i = 0; i < sizeof (hex); ++i) {
        hextab[(short)hex[i]] = i;
        if (isupper((short)hex[i])) hextab[(short)tolower((short)hex[i])] = i;
    }

    utf7mode = 0;
    utf8total = 0;
    bitstogo = 0;
    while ((c = *src) != '\0') {
        ++src;
        /* undo hex-encoding */
        if (c == '%' && src[0] != '\0' && src[1] != '\0') {
            c = (hextab[(short)src[0]] << 4) | hextab[(short)src[1]];
            src += 2;
        }
        /* normal character? */
        if (c >= ' ' && c <= '~') {
            /* switch out of UTF-7 mode */
            if (utf7mode) {
                if (bitstogo) {
                *dst++ = base64chars[(bitbuf << (6 - bitstogo)) & 0x3F];
                }
                *dst++ = '-';
                utf7mode = 0;
            }
            *dst++ = c;
            /* encode '&' as '&-' */
            if (c == '&') {
                *dst++ = '-';
            }
            continue;
        }
        /* switch to UTF-7 mode */
        if (!utf7mode) {
            *dst++ = '&';
            utf7mode = 1;
        }
        /* Encode US-ASCII characters as themselves */
        if (c < 0x80) {
            ucs4 = c;
            utf8total = 1;
        } else if (utf8total) {
            /* save UTF8 bits into UCS4 */
            ucs4 = (ucs4 << 6) | (c & 0x3FUL);
            if (++utf8pos < utf8total) {
                continue;
            }
        } else {
            utf8pos = 1;
            if (c < 0xE0) {
                utf8total = 2;
                ucs4 = c & 0x1F;
            } else if (c < 0xF0) {
                utf8total = 3;
                ucs4 = c & 0x0F;
            } else {
                /* NOTE: can't convert UTF8 sequences longer than 4 */
                utf8total = 4;
                ucs4 = c & 0x03;
            }
            continue;
        }
        /* loop to split ucs4 into two utf16 chars if necessary */
        utf8total = 0;
        do {
            if (ucs4 >= UTF16BASE) {
                ucs4 -= UTF16BASE;
                bitbuf = (bitbuf << 16) | ((ucs4 >> UTF16SHIFT)
                                           + UTF16HIGHSTART);
                ucs4 = (ucs4 & UTF16MASK) + UTF16LOSTART;
                utf16flag = 1;
            } else {
                bitbuf = (bitbuf << 16) | ucs4;
                utf16flag = 0;
            }
            bitstogo += 16;
            /* spew out base64 */
            while (bitstogo >= 6) {
                bitstogo -= 6;
                *dst++ = base64chars[(bitstogo ? (bitbuf >> bitstogo)
                               : bitbuf)
                                     & 0x3F];
            }
        } while (utf16flag);
    }
    /* if in UTF-7 mode, finish in ASCII */
    if (utf7mode) {
        if (bitstogo) {
            *dst++ = base64chars[(bitbuf << (6 - bitstogo)) & 0x3F];
        }
        *dst++ = '-';
    }
    /* tie off string */
    *dst = '\0';
}

