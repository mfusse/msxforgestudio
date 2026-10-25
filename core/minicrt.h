#ifndef MINICRT_H
#define MINICRT_H
#include <stddef.h>
void *memset(void*,int,size_t);
void *memcpy(void*,const void*,size_t);
int strcmp(const char*,const char*);
#endif
