#ifndef EVAL_H_INCLUDED
#define EVAL_H_INCLUDED

#include "var.h"

void stateInit(GlobalState*);
void parseModule(GlobalState*, char*);
void evaluate(GlobalState*);

int compare(Var, Var);
size_t toHash(Var);
void toString(Var, void*, size_t);
Var eval(GlobalState*, Var);

#endif // EVAL_H_INCLUDED
