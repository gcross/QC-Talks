#include <string.h>
#include <assert.h>
#include <stdio.h>

/*@only@*/ char *
fmt_buffer(const char *buf, int targetwidth) {
  char *newbuf = strdup(buf);
  size_t len = strlen(buf);
  unsigned int i = 0;
  unsigned int seeker;
  assert(newbuf != NULL);
  for(i=0; len > targetwidth + i; i=seeker+1) {
    for(seeker = i + targetwidth; 
        newbuf[seeker] != ' ' && newbuf[seeker] != '\t' && seeker > i; 
        seeker--);
    if(seeker > i) 
      newbuf[seeker] = '\n';
    else {
      for(seeker = i + targetwidth; 
          newbuf[seeker] != ' ' && newbuf[seeker] != '\t' && seeker < len-1; 
          seeker++);
      if(seeker < len-1)
        newbuf[seeker] = '\n';
      else {
        fprintf(stderr, "out early\n");
        return newbuf;
      }
    }
  }
  return newbuf;
}

