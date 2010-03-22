/* Copyright (c) 2002 Neil Spring and the University of Washington. *
 * Distributed under the terms of the GNU General Public License,   *
 * version 2.  See the file COPYING for more details.               *
 * This software comes with NO warranty.                            *
 * end license */

void Encode_Base64(/*@notnull@*/ const unsigned char *src, unsigned int srclen, 
                   /*@notnull@*/ /*@out@*/ unsigned char *dst);
/* returns how many bytes decoded */
unsigned int Decode_Base64(/*@notnull@*/ const unsigned char *src, 
                           /*@notnull@*/ /*@out@*/ unsigned char *dst); 
