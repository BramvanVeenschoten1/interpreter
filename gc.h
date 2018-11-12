#ifndef GC_H_INCLUDED
#define GC_H_INCLUDED

#include "var.h"

void unmark(GlobalState* state);
void mark(Var v);
void sweep(GlobalState* state);

void* _malloc(size_t);
void* _realloc(void*, size_t);
void _free(void*);
void* new(GlobalState* state, Type t);
void memPrint(GlobalState* state);

#endif // GC_H_INCLUDED
