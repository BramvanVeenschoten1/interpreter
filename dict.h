#ifndef DICT_H_INCLUDED
#define DICT_H_INCLUDED

#include "var.h"

void dictSet(Dict* d, Var key, Var value);
Var* dictGet(Dict* d, Var key);
void dictRemove(Dict* d, Var key);

int dictCompare(Dict* d, Dict* e);
size_t dictToHash(Dict* d);
void dictToString(Dict* d, String* out, size_t indent);
Dict* dictEval(GlobalState* state, Array* literal);

#endif // DICT_H_INCLUDED
