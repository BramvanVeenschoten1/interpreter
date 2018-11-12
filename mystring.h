#ifndef MYSTRING_H_INCLUDED
#define MYSTRING_H_INCLUDED

#include "var.h"

void stringAppend(String* s, String* t);
void stringReserve(String* s, size_t extra);
void quote(String* s);

int stringCompare(String* a, String* b);
size_t stringToHash(String* s);
void stringToString(String* s, String* out, size_t indent);

String* stringFromChar(GlobalState* state, char* x);
String* stringFromFile(GlobalState* state, FILE* f);
String* stringDup(GlobalState* state, String* s);
String* stringCat(GlobalState* state, String* s, String* t);
String* stringSlice(GlobalState* state, String* s, size_t begin, size_t end);

#endif // MYSTRING_H_INCLUDED
