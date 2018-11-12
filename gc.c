#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "array.h"
#include "dict.h"
#include "eval.h"
#include "gc.h"
#include "mystring.h"
#include "var.h"

int bufCount = 0;

static void arrayMark(Array* a){
    if(!a->tag){
        a->tag = 1;
        for(int i = 0; i < a->length; i++){
            mark(a->data[i]);
        }
    }
}
static void dictMark(Dict* d){
    if(!d->tag){
        d->tag = 1;
        for(int i = 0; i < d->length; i++){
            if(d->data[i].key.type == NONE)
                continue;
            mark(d->data[i].key);
            mark(d->data[i].value);
        }
    }
}
static void functionMark(Function* f){
    if(!f->tag){
        f->tag = 1;
        arrayMark(f->params);
        arrayMark(f->code);
    }
}
static void callMark(Call* c){
    if(!c->tag){
        c->tag = 1;
        mark(c->expr);
        arrayMark(c->argLists);
    }
}
static void assignMark(Assign* a){
    if(!a->tag){
        a->tag = 1;
        a->id->tag = 1;
        mark(a->expr);
    }
}

void mark(Var v){
    switch(v.type){
    case STRING:
    case IDENTIFIER:  ((Block*)v.ptr)->tag = 1; return;
    case DICTEXPR:
    case ARRAY:       return arrayMark(v.ptr);
    case DICT:        return dictMark(v.ptr);
    case FUNCTION:    return functionMark(v.ptr);
    case CALL:        return callMark(v.ptr);
    case ASSIGNMENT:  return assignMark(v.ptr);
    default:          return;
    }
}
void unmark(GlobalState* state){
    Block* p = state->gc;
    while(p){
        p->tag = 0;
        p = p->next;
    }
}
void sweep(GlobalState* state){
    Block* p = state->gc;
    state->gc = NULL;
    int swept = 0;
    int alive = 0;
    while(p){
        if(!p->tag){
            swept++;
            Block* tmp = p->next;
            switch(p->type){
                case ARRAY:      _free(((Array*)p)->data);  free(p); break;
                case DICT:       _free(((Dict*)p)->data);   free(p); break;
                case STRING:     _free(((String*)p)->data); free(p); break;
                case FUNCTION:   free(p); break;
                case CALL:       free(p); break;
                case ASSIGNMENT: free(p); break;
                default: printf("Fatal Error: Memory corruption\n"); exit(0xcafef00d);
            }
            p = tmp;
        }
        else{
            alive++;
            Block* tmp = p;
            p = p->next;
            tmp->next = state->gc;
            state->gc = tmp;
        }
    }
    printf("GC collected %d objects, %d objects and %d buffers are alive.\n", swept, alive, bufCount);
}
void memPrint(GlobalState* state){
    int string = 0;
    int array = 0;
    int dict = 0;
    int function = 0;
    int assign = 0;
    int call = 0;

    Block* p = state->gc;
    while(p){
        switch(p->type){
            case STRING:     string++;   break;
            case ARRAY:      array++;    break;
            case DICT:       dict++;     break;
            case FUNCTION:   function++; break;
            case ASSIGNMENT: assign++;   break;
            case CALL:       call++;     break;
            default: printf("Fatal Error: Memory Corrupted\n"); exit(0xcafef00d);
        }
        p = p->next;
    }
    printf("Living objects: \nString:\t\t%d\nArray:\t\t%d\nDict:\t\t%d\nFunction:\t%d\nAssign:\t\t%d\nCall:\t\t%d\nBuffer:\t\t%d\n",
           string, array, dict, function, assign, call, bufCount);
}

void* _malloc(size_t size){
    bufCount++;
    void* buf = malloc(size);
    if(buf)
        return buf;
    printf("Fatal Error: out of memory\n");
    exit(0xdeadbeef);
}
void* _realloc(void* ptr, size_t size){
    if(!ptr)
        bufCount++;
    ptr = realloc(ptr, size);
    if(ptr)
        return ptr;
    printf("Fatal Error: out of memory (%d)\n", (int)size);
    exit(0xfeeefeee);
}
void _free(void* ptr){
    if(!ptr)
        return;
    bufCount--;
    free(ptr);
}
void* new(GlobalState* state, Type t){
    if(state->newCount > 100){
        unmark(state);
        dictMark(state->stack);
        arrayMark(state->tmp);
        arrayMark(state->module);
        sweep(state);
        state->newCount = 0;
    }
    state->newCount++;

    size_t size;
    switch(t){
        case ARRAY:       size = sizeof(Array);    break;
        case DICT:        size = sizeof(Dict);     break;
        case STRING:      size = sizeof(String);   break;
        case FUNCTION:    size = sizeof(Function); break;
        case CALL:        size = sizeof(Call);     break;
        case ASSIGNMENT:  size = sizeof(Assign);   break;
        default:          printf("Wrong Tag\n");   exit(0xdeadbeef);
    }
    Block* self = calloc(1, size);
    self->next = state->gc;
    state->gc = self;
    self->type = t;
    return self;
}
