#ifndef _PLUGIN_H_
#define _PLUGIN_H_


typedef struct
{
    // return FALSE or 0 is the plugin does not support specified type
    int (*isSupported)(char *type);
    // return FALSE or 0 if the execute command failed for specified info string
    int (*execute)(char *info, FILE *fs, FILE *fh);
} Plugin;


#endif // _PLUGIN_H_
