/*
 * Copyright (c) 2002
 * Neil Spring and the University of Washington.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author(s) may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _COMMANDO_H
#define _COMMANDO_H

#include "nscommon.h" /* boolean */

#ifndef __LCLINT__
#include <getopt.h> /* required_argument, etc. */
#else
# define no_argument		0
# define required_argument	1
# define optional_argument	2
#endif

typedef void (*commando_cb)(const char *argument, void *a);
struct commandos {
  /*@null@*/ const char *description;
  /*@null@*/ const char *long_name;
  char short_abbreviation;
  int has_arg; /* same as for getopt */
  /*@null@*/ commando_cb set_cb;
  /*@dependent @*//*@null@*/ void *value_address;
};

/* enclose this in braces, a decision made because
   of emacs indenting. */
#define END_COMMANDO NULL, NULL, '\0', 0, NULL, NULL 
#define HELP_COMMANDO(parent) \
  "output this usage message", "help", 'h', no_argument, commando_help, parent 

void commando_int(const char *int_arg, void *pint);
void commando_strdup(const char *str_arg, void *pstr);
void commando_boolean(const char *dummy, void *pb);
void commando_fourchar(const char *str_arg, void *ptr_to_constant);
void commando_unsupported(const char *dummy, void *pb);
void commando_help(const char *dummy, void *pcommandos);
void commando_usage(/*@null@*/const char *dummy, const struct commandos *c);
/* returns the argument number where processing stopped */
/* blame getopt for the argv being not-const */
int commando_parse(int argc, char **argv, const struct commandos *c);
/* for debuggin' */
void commando_print_argv(int argc, char *argv[]);

#endif
