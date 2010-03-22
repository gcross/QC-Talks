/* Copyright (c) 2002 Neil Spring and the University of Washington. *
 * Distributed under the terms of the GNU General Public License,   *
 * version 2.  See the file COPYING for more details.               *
 * This software comes with NO warranty.                            *
 * end license */

void fish_internal_removereadhook(int sd);
void fish_internal_readhook(int sd, void (*rdfn)(int));
void fish_internal_writehook(int sd, void (*wrfn)(int));
void fish_internal_removewritehook(int sd);
void fish_main( unsigned int busy_wait_usec );
void fish_finish(void);
