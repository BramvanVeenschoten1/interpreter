#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "array.h"
#include "dict.h"
#include "eval.h"
#include "gc.h"
#include "mystring.h"
#include "var.h"

#define HASH_CONSTANT (1 \
            |1ULL << 2 |1ULL << 3 |1ULL << 5 |1ULL << 7 |1ULL << 11|1ULL << 13\
            |1ULL << 17|1ULL << 19|1ULL << 23|1ULL << 29|1ULL << 31|1ULL << 37\
            |1ULL << 41|1ULL << 43|1ULL << 47|1ULL << 53|1ULL << 59|1ULL << 61)
#define MAX_DENSITY_NUM 2
#define MAX_DENSITY_DENOM 3
#define MIN_DENSITY_NUM 1
#define MIN_DENSITY_DENOM 6
#define GROWTH_FACTOR 2

static size_t hash(size_t prehash, size_t max){
    size_t log = 1;
    size_t counter = max;
    while(counter){
        counter >>= 1;
        log++;
    }
    return (  (HASH_CONSTANT*prehash) >> (8*sizeof(size_t) - log)  ) % max;
}
static void resize(Dict* d, size_t length){
    if(!length)
        length = 6;
    Pair* tmp = _malloc(length * sizeof(Pair));
    for(int i = 0; i < length; i++)
        tmp[i].key.type = NONE;
    for(int i = 0; i < d->length; i++){
        if(d->data[i].key.type == NONE)
            continue;
        Pair p = d->data[i];
        size_t h = hash(toHash(p.key), length);
        while(tmp[h].key.type != NONE){
            h++;
            h %= length;
        }
        tmp[h] = p;
    }
    _free(d->data);
    d->data = tmp;
    d->length = length;
}

void dictSet(Dict* d, Var key, Var value){
    if(key.type == NONE) return;
    if(d->count * MAX_DENSITY_DENOM >= d->length * MAX_DENSITY_NUM)
        resize(d, d->length * GROWTH_FACTOR);
    size_t h = hash(toHash(key), d->length);
    Pair* p = d->data + h;
    while(1){
        if(p->key.type == NONE){
            p->key = key;
            p->value = value;
            d->count++;
            return;
        }
        if(!compare(p->key, key)){
            p->key = key;
            p->value = value;
            return;
        }
        h++; h %= d->length;
        p = d->data + h;
    }
}
Var* dictGet(Dict* d, Var key){
    if(key.type == NONE || !d->data)
        return NULL;
    size_t h = hash(toHash(key), d->length);
    Pair* p = d->data + h;
    for(;;){
        if(p->key.type == NONE)
            return NULL;
        if(!compare(p->key, key))
            return &p->value;
        h++; h %= d->length;
        p = d->data + h;
    }
}
void dictRemove(Dict* d, Var key){
    if(key.type == NONE)
        return;
    size_t h = hash(toHash(key), d->length);
    Pair* p = d->data + h;
    while(1){
        if(p->key.type == NONE)
            return;
        if(!compare(p->key, key)){
            p->key.type = NONE;
            d->count--;
            if(d->count * MIN_DENSITY_DENOM <= d->length * MIN_DENSITY_NUM){
                resize(d, d->length / GROWTH_FACTOR);
            }
            return;
        }
        h++; h %= d->length;
        p = d->data + h;
    }
}

int dictCompare(Dict* d, Dict* e){
    if(d == e)
        return 0;
    if(d->count != e->count)
        return d->count - e->count;
    for(int i = 0; i < d->length; i++){
        if(d->data[i].key.type == NONE)
            continue;
        Var* v = dictGet(e, d->data[i].key);
        if(v == NULL)
            return -1;
        int cmp = compare(d->data[i].value, *v);
        if(cmp)
            return cmp;
    }
    return 0;
}
size_t dictToHash(Dict* d){
    size_t result = 0;
    for(int i = 0; i < d->length; i++){
        if(d->data[i].key.type == NONE)
            continue;
        result += toHash(d->data[i].key);
        result += toHash(d->data[i].value);
    }
    return result;
}
void dictToString(Dict* d, String* out, size_t indent){
    stringReserve(out, indent + 2);
    char* c = out->data + out->length;
    for(int i = 0; i < indent; i++){
        *c = ' '; c++;
    }
    *c = '{'; c++;
    if(!d || !d->count || !d->data){
        *c = '}'; c++;
        out->length += indent + 2;
        return;
    }
    *c = '\n'; c++;
    out->length += indent + 2;

    for(int i = 0; i < d->length; i++){
        if(d->data[i].key.type == NONE)
            continue;
        toString(d->data[i].key, out, indent + 2);
        stringReserve(out, 2);
        c = out->data + out->length;
        *c = ':'; c++;
        *c = '\n'; c++;
        out->length += 2;

        toString(d->data[i].value, out, indent + 4);
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
    *c = '}'; c++;
    out->length += indent + 1;
}

Dict* dictEval(GlobalState* state, Array* literal){
    Dict* d = new(state, DICT);
    arrayPush(state->tmp, varFrom(d, DICT));
    for(int i = 0; i < literal->length; i++){
        Var key = eval(state, *arrayGet(literal, i));
        i++;
        arrayPush(state->tmp, key);
        Var value = eval(state, *arrayGet(literal, i));
        arrayPop(state->tmp);
        dictSet(d, key, value);
    }
    arrayPop(state->tmp);
    return d;
}
