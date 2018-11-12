#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "array.h"
#include "dict.h"
#include "eval.h"
#include "gc.h"
#include "mystring.h"
#include "var.h"

void arrayPush(Array* a, Var v){
    if(a->length >= a->capacity){
        if(!a->capacity)
            a->capacity = 2;
        a->data = _realloc(a->data, (a->capacity *= 2) * sizeof(Var));
    }
    a->data[a->length] = v;
    a->length++;
}
void arrayPop(Array* a){
    a->length--;
    if(a->length * 4 <= a->capacity){
        if(!a->length){
            _free(a->data);
            a->data = NULL;
            a->capacity = 0;
        }
        else
            a->data = _realloc(a->data, (a->capacity /= 2) * sizeof(Var));
    }
}
void arrayAppend(Array* a, Array* b){
    if(!a || !b) return;
    if(a->capacity < a->length + b->length)
        a->data = _realloc(a->data, (a->capacity = a->length + b->length) * sizeof(Var));
    memcpy(a->data + a->length, b->data, b->length * sizeof(Var));
}
Var* arrayGet(Array* a, size_t i){
    if(i >= a->length){
        printf("Array out of bounds\n");
        return NULL;
    }
    return a->data + i;
}
Var* binarySearch(Array* d, Var k){
    if(!d) return NULL;
    if(!d->length) return NULL;

    size_t left = 0;
    size_t right = d->length - 1;
    size_t m = (left + right + 1) / 2;
    while(left < right){
        int cmp = compare(d->data[m], k);
        if(cmp < 0){
            right = m;
            m = (left + right) / 2;
        }
        if(cmp > 0){
            left = m;
            m = (left + right + 1) / 2;
        }
        if(!cmp){
            return d->data + m;
        }
    }
    return NULL;
}
static int sortCompare(const void* a, const void* b){
    return compare(*(Var*)a, *(Var*)b);
}
void arraySort(Array* d){
    qsort(d, d->length, sizeof(Var), sortCompare);
}

int arrayCompare(Array* a, Array* b){
    if(a->length - b->length)
        return a->length - b->length;
    for(size_t i = 0; i < a->length; i++){
        int c = compare(a->data[i], b->data[i]);
        if(c) return c;
    }
    return 0;
}
size_t arrayToHash(Array* a){
    size_t result = 0;
    for(int i = 0; i < a->length; i++)
        result += toHash(a->data[i]);
    return result;
}
void arrayToString(Array* a, String* out, size_t indent){
    stringReserve(out, indent + 2);

    char* c = out->data + out->length;
    for(int i = 0; i < indent; i++){
        *c = ' '; c++;
    }
    *c = '['; c++;
    if(!a || !a->length){
        *c = ']'; c++;
        out->length += indent + 2;
        return;
    }
    *c = '\n'; c++;
    out->length += indent + 2;

    for(int i = 0; i < a->length; i++){
        toString(a->data[i], out, indent + 2);
        stringReserve(out, 2);
        c = out->data + out->length;
        *c = ','; c++;
        *c = '\n'; c++;
        out->length += 2;
    }
    stringReserve(out, indent + 1);
    for(int i = 0; i < indent; i++){
        *c = ' '; c++;
    }
    *c = ']'; c++;
    out->length += indent + 1;
}
Array* arrayEval(GlobalState* state, Array* a){
    Array* self = new(state, ARRAY);
    arrayPush(state->tmp, varFrom(a, ARRAY));
    self->data = _malloc(a->capacity * sizeof(Var));
    self->capacity = a->capacity;
    self->length = a->length;
    for(int i = 0; i < a->length; i++)
        self->data[i] = eval(state, a->data[i]);
    arrayPop(state->tmp);
    return self;
}

Array* arraySlice(GlobalState* state, Array* a, size_t begin, size_t end){
    if(begin < 0 || begin >= end || begin >= a->length || end > a->length){
        printf("Array slice out of bounds\n");
        return NULL;
    }
    Array* self = new(state, ARRAY);
    self->data = _malloc((end - begin) * sizeof(Var));
    self->capacity = end - begin;
    self->length = end - begin;
    memcpy(self->data, a->data + begin, self->length * sizeof(Var));
    return self;
}
Array* arrayCat(GlobalState* state, Array* a, Array* b){
    Array* self = new(state, ARRAY);
    self->data = _malloc((a->length + b->length)* sizeof(Var));
    self->capacity = a->length + b->length;
    self->length = self->capacity;
    memcpy(self->data, a->data, a->length * sizeof(Var));
    arrayAppend(self, b);
    return self;
}
Array* arrayDup(GlobalState* state, Array* a){
    Array* self = new(state, ARRAY);
    self->data = _malloc(a->length* sizeof(Var));
    self->capacity = a->length;
    self->length = self->capacity;
    memcpy(self->data, a->data, a->length * sizeof(Var));
    return self;
}
