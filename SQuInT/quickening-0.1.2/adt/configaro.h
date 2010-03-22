#ifndef _configaro_h
#define _configaro_h
#include "nscommon.h"
#include "commando.h"
#include "lamefmt.h"

#define CFO(desc, structname, cb) { desc, #structname, &structname, cb, FALSE }
#define CFX(desc, structname, cb) { desc, #structname, &structname, cb, TRUE }
#define END_CONFIGARO  { NULL, NULL, NULL, NULL, FALSE }

struct configaro {
  const char *description;
  const char *name;
  /*@dependent@*/ void *value_address;
  commando_cb cb;
  boolean custom; /* whether this is set on install for this machine */
};

void 
configaro_write_current_config(struct configaro *cfg, int s);
boolean 
configaro_load_configuration(struct configaro *cfg, 
                             const char * filename);
boolean 
  configaro_is_customized(const struct configaro *cfg, const char *name);
void 
  configaro_set(struct configaro *cfg, const char *name, const char *value);
void 
configaro_write_default_config_file(struct configaro *cfg, 
                                    const char * filename);
void 
configaro_write_tex_description(struct configaro *cfg,
                                const char *texfilename,
                                int argc,
                                char **argv);
void configaro_boolean(const char *value, 
                       void *pb);
void configaro_header(const char *value,
                      UNUSED(void *pb)) FUNCATTR_NORETURN;

#endif
