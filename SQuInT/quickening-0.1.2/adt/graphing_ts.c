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

#undef DEBUG
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef GRAPHICS
#include <gtk/gtk.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "graphing_ts.h"
#include "nscommon.h"
#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#define HIST_LEN 512
struct graphing_timeseries_struct {
  struct pair {
    double val;
#ifdef timestamps
    struct timeval tv;
#endif
  } data[HIST_LEN];
  int index;
  const char *label;
  double max;
  
  int height, width;
  unsigned char *rgbbuf; // width * height * 3
  int pipes[2];
  GIOChannel *gioc;
  GtkWidget *window, *button, *vbox, *darea;
  GdkFont *font;
  pthread_mutex_t mutex;
};
gboolean
on_darea_expose (GtkWidget *widget,
                 GdkEventExpose *event,
                 gpointer user_data);

void *gtk_main_thread(void *dummy) {

  gdk_rgb_init();
  gtk_main();
  return(NULL);
}

#ifdef debug
#define UNLOCK \
do { \
  pthread_mutex_unlock(&ts->mutex); \
  fprintf(stderr, "deq: %s:%d\n", __FILE__, __LINE);  \
} while(0);
#define LOCK \
do { \
  fprintf(stderr, "acq: %s:%d\n", __FILE__, __LINE);  \
  pthread_mutex_lock(&ts->mutex); \
} while(0);
#else
#define UNLOCK pthread_mutex_unlock(&ts->mutex); 
#define LOCK pthread_mutex_lock(&ts->mutex);
#endif

static gint configure_event(GtkWidget *widget, 
                            GdkEventConfigure *event,
                            gpointer data) {
  graphing_timeseries ts = (graphing_timeseries)data;
  LOCK;
  if(ts->rgbbuf)
    free(ts->rgbbuf);
  ts->height=widget->allocation.height;
  ts->width=MIN(widget->allocation.width,HIST_LEN);
  ts->rgbbuf=(unsigned char *)malloc(ts->height * ts->width * 3);
  UNLOCK;
  return(FALSE);
}

gint delete_event( GtkWidget *widget,
                   GdkEvent  *event,
                   gpointer   data ) {
  // gtk_main_quit();
  g_print("delete! aaaaah");
  return(FALSE);
}

static gboolean skank(GIOChannel *source,
                      GIOCondition condition,
                      gpointer data) {
  char buf[200];
  int read_bytes;
  graphing_timeseries ts = (graphing_timeseries)data;
  
  // g_print("hooha!\n");
  g_io_channel_read(source,
                    buf,
                    200,
                    &read_bytes);
  if(read_bytes == 4 && strcmp(buf,"exit") == 0) {
    gtk_main_quit();
  } else {
    gtk_widget_draw(ts->darea,NULL);
  }
  // g_print("teepee!\n");
  return(TRUE);
}

pthread_t gtk_thread;

graphing_timeseries new_graphing_timeseries(const char *label) {
  graphing_timeseries ts = (graphing_timeseries) 
    malloc(sizeof(struct graphing_timeseries_struct));
  memset(ts, 0, sizeof(struct graphing_timeseries_struct));
  pthread_mutex_init(&ts->mutex, NULL);
  ts->label = label;
  ts->height=128;
  ts->width=128;
  ts->max=0.0;

  ts->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  ts->vbox = gtk_vbox_new (FALSE, 0);
  //ts->button = gtk_button_new_with_label (label);
  ts->button = gtk_label_new (label);
  ts->darea = gtk_drawing_area_new();
  gtk_drawing_area_size (GTK_DRAWING_AREA (ts->darea), ts->width, ts->height);
  gtk_container_add (GTK_CONTAINER (ts->window), ts->vbox);
  gtk_container_add (GTK_CONTAINER (ts->vbox), ts->darea);
  gtk_container_add (GTK_CONTAINER (ts->vbox), ts->button);
  gtk_signal_connect (GTK_OBJECT (ts->window), "delete_event",
                      GTK_SIGNAL_FUNC (delete_event), NULL);
  gtk_signal_connect (GTK_OBJECT (ts->darea), "expose-event",
                      GTK_SIGNAL_FUNC (on_darea_expose), ts);
  gtk_signal_connect (GTK_OBJECT (ts->darea), "configure-event",
                      GTK_SIGNAL_FUNC (configure_event), ts);
  gtk_widget_show_all (ts->window);
  assert(pipe(ts->pipes)==0);
  ts->gioc = g_io_channel_unix_new(ts->pipes[0]);
  g_io_add_watch_full(ts->gioc, 0, G_IO_IN, skank, ts, NULL);
  ts->font = 
    gdk_font_load("-misc-fixed-medium-r-normal--*-100-*-*-c-*-iso8859-*");

  if(gtk_thread == 0) 
    pthread_create(&gtk_thread, NULL, gtk_main_thread, NULL);

  return(ts);
}

static void gts_fill_buffer (graphing_timeseries ts);

void gts_add(graphing_timeseries ts, double estimate, struct timeval *tv) {
  DEBUG_PRINT(("gts acquire\n"));
  LOCK;
  DEBUG_PRINT(("gts acquired\n"));
  ts->index= (ts->index+1)%HIST_LEN;
  ts->data[++ts->index].val = estimate;
#ifdef timestamps
  ts->data[  ts->index].tv.tv_sec = tv->tv_sec;
  ts->data[  ts->index].tv.tv_usec = tv->tv_usec;
#endif
  ts->max = MAX(estimate, ts->max);
  assert(ts->max < 200000000.0);
  //  gdk_threads_enter();
  gts_fill_buffer(ts);
  // g_print("hock\n");
  write(ts->pipes[1], "boo", strlen("boo"));
  UNLOCK;
  // gdk_threads_leave();

}

static void gts_fill_buffer (graphing_timeseries ts){
  int i,j;
  unsigned char *pos = ts->rgbbuf;
  double newmax = 0.0;
  DEBUG_PRINT(("filling buffer\n"));
  // origin is at upper left in graphics display... farble.
  for(i=0; i<MIN(ts->width,HIST_LEN); i++) {
    newmax = MAX(newmax, 
                 ts->data[(ts->index 
                           - ts->width 
                           + i 
                           + HIST_LEN)
                         %HIST_LEN].val);
    assert(newmax < 200000000.0);
  }
  ts->max = newmax;
  if(ts->max > DBL_EPSILON) {
    for(j=ts->height; j>0; j--) {
      for(i=0; i<MIN(ts->width,HIST_LEN); i++) {
        // this should be made cheaper, I think.
        if(j<((ts->data[(ts->index 
                         - ts->width 
                         + i 
                         + HIST_LEN)
                       %HIST_LEN].val)/ts->max)*ts->height) {
          *pos++ = 32;    /* Red. */
          *pos++ = 212;   /* Green. */
          *pos++ = 212;   /* Blue. */
        } else {
          *pos++ = 0;   /* Red. */
          *pos++ = 0;   /* Green. */
          *pos++ = 0;   /* Blue. */
        }
      }
    }
  }
  
}


gboolean
on_darea_expose (GtkWidget *widget,
                 GdkEventExpose *event,
                 gpointer user_data)
{
  char buf[48];
  graphing_timeseries ts = user_data;
  LOCK;
  // g_print("exposed\n");
  gdk_draw_rgb_image (widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
                      0, 0, ts->width, ts->height,
                      GDK_RGB_DITHER_MAX, ts->rgbbuf, ts->width * 3);
  assert(ts->max < 200000000.0);
  
  if(ts->max < 1000) {
    snprintf(buf, 48, "%5.1f %s/s", 
             ts->max, "B");
  } else {
    snprintf(buf, 48, "%5.1f %s/s", 
             ts->max/1024.0, "KB");
  }
  gdk_draw_string(widget->window, 
                  ts->font, widget->style->white_gc,
                  0, 12, buf);
  UNLOCK;
  // only if this is false will the thing render
  return(FALSE);
}

void gts_bail(graphing_timeseries ts) {
  write(ts->pipes[1], "exit", strlen("exit"));
}
#endif
