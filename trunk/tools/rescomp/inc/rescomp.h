#ifndef _RESCOMP_H_
#define _RESCOMP_H_


#ifndef NULL
#define NULL            0
#endif

#ifndef FALSE
#define FALSE           0
#endif
#ifndef TRUE
#define TRUE            1
#endif


#define MAX_PATH_LEN    2048
#define MAX_LINE_LEN    2048
#define MAX_NAME_LEN    32


// keep trace of the app and input file directories
extern char *currentDirSystem;
extern char *currentDir;
extern char *resDirSystem;
extern char *resDir;


#endif // _RESCOMP_H_
