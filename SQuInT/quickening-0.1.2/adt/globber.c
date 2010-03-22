#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <glob.h>
#include "nscommon.h"
#include "globber.h"

boolean glob_each(const char *glob_expression, boolean (*each_callback)(const char *filename, void *a), void *a) {
  glob_t globber;
  int zok;            /* zero means ok */
  size_t i; 

  zok = glob(glob_expression, 
#ifdef GLOB_BRACE
             /* solaris lacks GLOB_BRACE for some reason.
                too bad, I really like the braces. */
             GLOB_BRACE | 
#endif
             GLOB_NOSORT, NULL, &globber);
  if(zok == 0) {
    for (i=0; i< globber.gl_pathc; i++) {
      if( each_callback(globber.gl_pathv[i], a) ) {
        globfree(&globber);
        return TRUE;
      }
    }
    globfree(&globber);
  }
  return(FALSE);
}
