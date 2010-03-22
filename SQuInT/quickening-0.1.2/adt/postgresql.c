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
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#if defined (HAVE_LIBPQ) && (defined(HAVE_POSTGRESQL_LIBPQ_FE_H) || defined(HAVE_LIBPQ_FE_H))
#include "postgresql.h" /* includes the other stuff */
#include "progress.h"

/* when you don't care about the result, just perhaps
   that it succeeded.  "alt void" keeps lclint from 
   complaining if we don't check the return code */;
boolean  /*@alt void@*/
db_do_command(PGconn *conn, const char *querybuf) {
  static int reprieves;
  PGresult   *res;
  res = PQexec(conn, querybuf);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    printf("failed to %s\n", querybuf);
    printf("res->status = %s\n", PQresStatus(PQresultStatus(res))); 
    PQclear(res);
    if(reprieves++ > 5) {
      exit(EXIT_FAILURE);
    }
    return FALSE;
  }
  PQclear(res);
  return TRUE;
}
static boolean 
db_do_query_internal(PGconn *conn, const char *querybuf,
                     boolean showProgress,
                     void (*row_cb)(const char **, void *a),
                     void *a, 
                     boolean exit_on_failure) {

  PGresult *res = PQexec(conn, querybuf);
  if (PQresultStatus(res) != PGRES_TUPLES_OK && 
      PQresultStatus(res) != PGRES_EMPTY_QUERY) {
    printf("db failed: %s\n", querybuf);
    printf(" res->status = %s\n", PQresStatus(PQresultStatus(res))); 
    printf(" error message= %s\n", PQerrorMessage(conn)); 
    if(exit_on_failure) exit(EXIT_FAILURE); else return FALSE;
  }
  if(row_cb != NULL) {
    int nFields = PQnfields(res);
    int nTuples = PQntuples(res); 
    const char **pass = malloc(sizeof(char *) * nFields);
    int i;
    for (i = 0; i < nTuples; i++) {
      int j;
      if(showProgress) progress_n_of(i, nTuples);
      for(j=0;j<nFields;j++) {
        pass[j] = PQgetvalue(res,i,j);
      }
      row_cb(pass, a);
    } 
    if(showProgress) progress_n_of(nTuples, nTuples);
    free(pass);
  }
  PQclear(res);
  return TRUE;
}

boolean 
db_do_query(PGconn *conn, const char *querybuf,
            void (*row_cb)(const char **, void *a),
            void *a,
            boolean exit_on_failure) {
  return db_do_query_internal(conn, querybuf, FALSE, row_cb, a, exit_on_failure);
}
boolean 
db_do_query_progress(PGconn *conn, const char *querybuf,
                     void (*row_cb)(const char **, void *a),
                     void *a,
                     boolean exit_on_failure) {
  return db_do_query_internal(conn, querybuf, TRUE, row_cb, a, exit_on_failure);
}

boolean db_begin_transaction(PGconn *conn) {
  return db_do_command(conn, "begin");
}
boolean db_commit_transaction(PGconn *conn) {
  /* could check nesting or something ? */
  return db_do_command(conn, "commit");
}

PGconn *db_connect(const char *database) {
  static PGconn *conn;  /* cache the connection, if already made */
  if(conn == NULL) {
    char connectstring[256];
    sprintf(connectstring, 
            "host=jimbo.cs.washington.edu dbname=%s user=%s", 
            database, getenv("USER")); 
    conn = PQconnectdb(connectstring);
  }
  if(PQstatus(conn) != CONNECTION_OK) {
    fprintf(stderr, "warning, unable to connect to database.\n");
    fprintf(stderr, "%s\n", PQerrorMessage(conn));
  }
  return (conn);
}
#endif

