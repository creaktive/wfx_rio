#ifndef _parsecfg_public
#define _parsecfg_public

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define LINELEN		255

int parsecfg (char *filename, char comment, int process(char *key, char *val));

#endif
