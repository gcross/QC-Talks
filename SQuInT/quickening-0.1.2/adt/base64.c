/* Copyright (c) 2002 Neil Spring and the University of Washington. *
 * Distributed under the terms of the GNU General Public License,   *
 * version 2.  See the file COPYING for more details.               *
 * This software comes with NO warranty.                            *
 * end license */
#include "base64.h"
#include <stdlib.h>

#define PAD '='
#define UCZ ((unsigned char)'\0')

/*@observer@*/ const unsigned char *ALPHABET = (unsigned char *)
     "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/\000";

     /* find char in in alphabet, return offset, -1 otherwise */
     int find_char(unsigned char c)
{
  const unsigned char *a = ALPHABET;

  while (*a != UCZ)
    if (*(a++) == c)
      return a - ALPHABET - 1;

  return -1;
}


void Encode_Base64(/*@notnull@*/ const unsigned char *src, unsigned int srclen, 
                   /*@notnull@*//*@out@*/ unsigned char *dst)
{
  unsigned int g = 0;
  unsigned int c = 0;
  const unsigned char *maxsrc = src + srclen;

  /* if (src == NULL || dst == NULL) return; */

  while (src < maxsrc) {
    g = (g << 8) | *src++;
    if (c == 2) {
      *dst++ = ALPHABET[0x3f & (g >> 18)];
      *dst++ = ALPHABET[0x3f & (g >> 12)];
      *dst++ = ALPHABET[0x3f & (g >> 6)];
      *dst++ = ALPHABET[0x3f & g];
    }
    c = (c + 1) % 3;
  }

  if (c != 0) {
    if (c == 1) {
      *dst++ = ALPHABET[0x3f & (g >> 2)];
      *dst++ = ALPHABET[0x3f & (g << 4)];
      *dst++ = PAD;
      *dst++ = PAD;
    } else {
      *dst++ = ALPHABET[0x3f & (g >> 10)];
      *dst++ = ALPHABET[0x3f & (g >> 4)];
      *dst++ = ALPHABET[0x3f & (g << 2)];
      *dst++ = PAD;
    }
  }
  *dst = UCZ;
}


unsigned int Decode_Base64(/*@notnull@*/ const unsigned char *src, 
                           /*@notnull@*/ unsigned char *dst)
{
  unsigned int g = 0;
  unsigned int c = 0;
  int n = 0;
  unsigned char *origdst = dst;

  /* if (!src || !dst) return 0; */

  while (*src != UCZ) {
    n = find_char(*src++);
    if (n < 0)
      continue;

    g <<= 6;
    g |= n;
    if (c == 3) {
      *dst++ = (unsigned char) (g >> 16);
      *dst++ = (unsigned char) (g >> 8);
      *dst++ = (unsigned char) (g);
      g = 0;
    }
    c = (c + 1) % 4;
  }
  if (c != 0) {
    if (c == 1) {
      /* shouldn't happen, but do something anyway */
      *dst++ = (unsigned char)(g << 2);
    } else if (c == 2) {
      *dst++ = (unsigned char)(g >> 4);
    } else {
      *dst++ = (unsigned char)(g >> 10);
      *dst++ = (unsigned char)(g >> 2);
    }
  }
  *dst = UCZ;
  return((unsigned int) (dst-origdst));
}


