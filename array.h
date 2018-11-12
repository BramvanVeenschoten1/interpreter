#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include "var.h"

void arrayPush(Array* a, Var v);
void arrayPop(Array* a);
void arrayAppend(Array* a, Array* b);
Var* arrayGet(Array* a, size_t i);
void arraySort(Array* a);
Var* binarySearch(Array* a, Var v);

int arrayCompare(Array* a, Array* b);
size_t arrayToHash(Array* a);
void arrayToString(Array* a, String* out, size_t indent);
Array* arrayEval(GlobalState* state, Array* a);
Array* arrayDup(GlobalState* state, Array* a);
Array* arrayCat(GlobalState* state, Array* a, Array* b);
Array* arraySlice(GlobalState* state, Array* a, size_t begin, size_t end);

#endif // ARRAY_H_INCLUDED
