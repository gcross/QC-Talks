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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "commando.h"
#ifndef __LCLINT__
#include <getopt.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#ifdef __LCLINT__
int getopt_long(int argc, char * const argv[],
                const char *optstring,
                /*@null@*/const struct option *longopts, 
                /*@null@*/int *longindex);
# define no_argument		0
# define required_argument	1
# define optional_argument	2
struct option {
  /*@dependent@*/ const char *name;
  int has_arg;
  /*@null@*/ int *flag;
  int val;
};
#endif


void commando_int(const char *int_arg, void *pint) {
  int *p = (int *)pint;
  *p = atoi(int_arg);
}

void commando_strdup(const char *str_arg, void *pstr) {
  char **p = (char **)pstr;
  *p = strdup(str_arg);
}

void commando_boolean(/*@unused@*/const char *dummy __attribute__((unused)), 
                      void *pb) {
  boolean *p = (boolean *)pb;
  *p = TRUE;
}

/* apple tends to use four character named constants that
   turn into integers, this allows the specification of such
   a constant.  */
void commando_fourchar(const char *str_arg, void *ptr_to_constant) {
  char *ctype = ptr_to_constant;
  if(strlen(str_arg) > 4 || strlen(str_arg) < 3) {
    printf("wrong length for constant: %s\n", str_arg);
    exit(EXIT_FAILURE);
  }
  ctype[0]=str_arg[0];
  ctype[1]=str_arg[1];
  ctype[2]=str_arg[2];
  ctype[3]= (str_arg[3] == '\0') ? ' ' : str_arg[3]; 
}

/* plenty to be done here wrt formatting */
void commando_usage(/*@null@*//*@unused@*/
                    const char *dummy __attribute__((unused)),
                    const struct commandos *c) FUNCATTR_NORETURN;
void commando_usage(/*@null@*//*@unused@*/
                    const char *dummy __attribute__((unused)),
                    const struct commandos *c) {
  int i;
  printf("Usage:\n");
  for(i=0; c[i].description != NULL; i++) {
    printf("  -%c, --%s%s\n", c[i].short_abbreviation, 
           c[i].long_name, (c[i].has_arg!=no_argument)? "=arg" : "");
    printf("      %s\n", c[i].description);
  }
  printf("\n");
  exit(EXIT_SUCCESS);
}
void commando_help(/*@unused@*/ const char *dummy __attribute__((unused)), 
                   void *pcommandos) FUNCATTR_NORETURN;
void commando_help(/*@unused@*/ const char *dummy __attribute__((unused)), 
                   void *pcommandos) {
  commando_usage(NULL, pcommandos);
}

void commando_unsupported(/*@unused@*/ 
                          const char *dummy __attribute__((unused)), 
                          /*@unused@*/
                          void *pb __attribute__((unused))) FUNCATTR_NORETURN;
void commando_unsupported(/*@unused@*/ 
                          const char *dummy __attribute__((unused)), 
                          /*@unused@*/
                          void *pb __attribute__((unused))) {
  printf("an unsupported option was requested\n");
  exit(EXIT_FAILURE);
}

int commando_parse(int argc, char **argv, const struct commandos *c) {
  /* marshal everything for use with gnu getopt_long. it's a
   mess, but I don't really want to rewrite getopt myself */
  int i;
  int chararg;
  int longindex;
  char options_string[128];
  char *p;
  /* first, fill up the character options string. */
  struct option *long_options;

  /* todo: bounds check */
  memset(options_string, 0, sizeof(options_string));
  options_string[0] = '+';
  for(i=0, p=(options_string+1); c[i].description != NULL; i++) {
    if(c[i].short_abbreviation != '\0') {
      if(strchr(options_string, c[i].short_abbreviation)) {
        printf("WARNING: -%c is an ambiguous short-option!\n", 
               c[i].short_abbreviation);
      }
      *(p++) = c[i].short_abbreviation;
      if(c[i].has_arg != no_argument) {
        *(p++) = ':';
      }
    }
  }
  *p = '\0';
  /* i now is the number of entries */
  long_options = malloc((i+1) * sizeof(struct option));
  assert(long_options != NULL);
  for(i=0; c[i].description != NULL; i++) {
    long_options[i].name = c[i].long_name;
    long_options[i].has_arg = c[i].has_arg;
    long_options[i].flag = NULL;
    long_options[i].val = (int) c[i].short_abbreviation;
  }
  long_options[i].name = NULL;
  long_options[i].flag = NULL;
  long_options[i].has_arg = long_options[i].val = 0;

  /* all set up. */
  while((chararg = getopt_long(argc, argv,
                               options_string, 
                               long_options, 
                               &longindex)) != EOF) {
    if(chararg == 0) {
      c[longindex].set_cb(optarg, c[longindex].value_address);
    } else {
      for(i=0; c[i].description != NULL &&
            chararg != (int)c[i].short_abbreviation ; i++);
      if(chararg == (int)c[i].short_abbreviation) {
        c[i].set_cb(optarg, c[i].value_address);
      } else {
        commando_usage(NULL, c);
      }
    }
  }
  free(long_options);
  return(optind);
}

void commando_print_argv(int argc, char *argv[]) {
  int i;
  for(i=0; i<argc; i++) {
    fprintf(stderr, "%d: %s\n", i, argv[i]);
  }
}
