#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "array.h"
#include "dict.h"
#include "eval.h"
#include "gc.h"
#include "mystring.h"
#include "var.h"

union Hash {
    size_t result;
    char bytes[0];
};

void stringAppend(String* s, String* t){
    if(!t || !s) return;
    if(s->capacity < s->length + t->length)
        s->data = _realloc(s->data, s->capacity = s->length + t->length);
    memcpy(s->data + s->length, t->data, t->length);
    s->length += t->length;
}
void stringReserve(String* s, size_t extra){
    if(s->capacity < s->length + extra){
        if(s->capacity * 2 < s->length + extra)
            s->data = _realloc(s->data, s->capacity = s->length + extra);
        else
            s->data = _realloc(s->data, s->capacity *= 2);
    }
}
void quote(String* s){
    for(int i = 0; i < s->length; i++)
        putchar(s->data[i]);
    putchar('\n');
}

int stringCompare(String* a, String* b){
    if(a == b) return 0;
    if(a->length != b->length) return a->length - b->length;
    for(int i = 0; i < a->length; i++)
        if(a->data[i] != b->data[i]) return a->data[i] - b->data[i];
    return 0;
}
size_t stringToHash(String* s){
    union Hash h;
    h.result = 0;

    int index = 0;
    for(int i = 0; i < s->length; i++){
        h.bytes[index] += s->data[i];
        index++; index %= 8;
    }
    return h.result;
}
void stringToString(String* s, String* out, size_t indent){
    size_t length = s->length + indent;
    stringReserve(out, length);
    memset(out->data + out->length, ' ', indent);
    out->length += indent;
    memcpy(out->data + out->length, s->data, s->length);
    out->length += s->length;
}

String* stringFromChar(GlobalState* state, char* x){
    size_t length = strlen(x);
    String* self = new(state, STRING);
    self->data = _malloc(length);
    self->capacity = length;
    self->length = length;
    memcpy(self->data, x, length);
    return self;
}
String* stringFromFile(GlobalState* state, FILE* f){
    fseek(f, 0, SEEK_END);
    size_t length = ftell(f);
    fseek(f, 0, SEEK_SET);

    String* self = new(state, STRING);
    self->data = _malloc(length);
    self->capacity = length;

    int c = fgetc(f);
    int i = 0;
    while(c != -1){
        self->data[i] = c;
        c = fgetc(f);
        i++;
    }
    self->length = i;
    return self;
}
String* stringDup(GlobalState* state, String* s){
    String* self = new(state, STRING);
    self->data = _malloc(s->length);
    self->capacity = s->length;
    self->length = s->length;
    memcpy(self->data, s->data, s->length);
    return self;
}
String* stringCat(GlobalState* state, String* s, String* t){
    String* self = new(state, STRING);
    self->data = _malloc(s->length + t->length);
    self->capacity = s->length + t->length;
    self->length = s->length;
    memcpy(self->data, s->data, s->length);
    stringAppend(self, t);
    return self;
}
String* stringSlice(GlobalState* state, String* s, size_t begin, size_t end){
    if(begin < 0 || begin >= end || begin >= s->length || end > s->length){
        printf("String slice out of bounds\n");
        return NULL;
    }
    String* self = new(state, STRING);
    self->data = _malloc(end - begin);
    self->capacity = end - begin;
    self->length = end - begin;
    memcpy(self->data, s->data + begin, self->length);
    return self;
}

Identifier* identifierFromChar(GlobalState* state, char* x){
    size_t length = strlen(x);
    Identifier* self = new(state, IDENTIFIER);
    self->source = (void*)self;
    self->data = x;
    self->length = length;
    return self;
}
int identifierCompare(Identifier* a, Identifier* b){
    if(a == b)
        return 0;
    if(a->length != b->length)
        return a->length - b->length;
    for(int i = 0; i < a->length; i++){
        if(a->data[i] != b->data[i])
            return a->data[i] - b->data[i];
    }
    return 0;
}
size_t identifierToHash(Identifier* a){
    union Hash h;
    h.result = 0;

    int index = 0;
    for(int i = 0; i < a->length; i++){
        h.bytes[index] += a->data[i];
        index++; index %= 8;
    }
    return h.result;
}
