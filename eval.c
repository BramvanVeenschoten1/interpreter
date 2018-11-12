#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "array.h"
#include "dict.h"
#include "eval.h"
#include "gc.h"
#include "mystring.h"
#include "var.h"

static void functionToString(Function* f, String* out, size_t indent){
    stringReserve(out, indent + 12);
    memset(out->data + out->length, ' ', indent);
    out->length += indent;
    memcpy(out->data + out->length, "__function__", 12);
    out->length += 12;
}
static void builtinToString(String* out, size_t indent){
    stringReserve(out, indent + 11);
    memset(out->data + out->length, ' ', indent);
    out->length += indent;
    memcpy(out->data + out->length, "__builtin__", 11);
    out->length += 11;
}
static void floatToString(double d, String* out, size_t indent){
    char tmp[64];
    memset(tmp, ' ', indent);
    sprintf(tmp + indent, "%g", d);
    size_t length = strlen(tmp);
    stringReserve(out, length);
    memcpy(out->data + out->length, tmp, length);
    out->length += length;
}
static void intToString(long long _int, String* out, size_t indent){
    char tmp[64];
    memset(tmp, ' ', indent);
    sprintf(tmp + indent, "%d", (int)_int);
    size_t length = strlen(tmp);
    stringReserve(out, length);
    memcpy(out->data + out->length, tmp, length);
    out->length += length;
}

static Var evalAssign(GlobalState* state, Assign* a){
    Var key = varFrom(a->id, IDENTIFIER);
    Var value = eval(state, a->expr);
    if(value.type == ERROR){
        printf("(line %d) while evaluating assignment\n", (int)a->lineNo);
        return value;
    }
    dictSet(state->stack, key, value);
    return value;
}
static Var evalIdentifier(GlobalState* state, String* s){
    Var key = varFrom(s, IDENTIFIER);
    Var* result;
    Dict* frame = state->stack;
    for(;;){
        result = dictGet(frame, key);
        if(result)
            return *result;
        result = dictGet(frame, varFrom(7, INT));
        if(result)
            frame = result->ptr;
        else{
            printf("Error: Unknown identifier: ");
            quote(s);
            return varFrom(0, ERROR);
        }
    }
}
static Var evalCall(GlobalState* state, Call* c){
    Var result = eval(state, c->expr);

    for(int i = 0; i < c->argLists->length; i++){
        Var callable = result;

        Array* args = arrayGet(c->argLists, i)->ptr;
        args = arrayEval(state, args);
        arrayPush(state->tmp, varFrom(args, ARRAY));

        if(callable.type == BUILTIN){
            result = ((  Var(*)(GlobalState*, Array*)  )callable.ptr)(state, args);
        }
        else if (callable.type == FUNCTION){
            Function* f = callable.ptr;
            Array* params = f->params;
            if(params->length != args->length){
                printf("Expected %d arguments, got %d\n", (int)params->length, (int)args->length);
                return varFrom(0, ERROR);
            }

            Dict* frame = new(state, DICT);
            dictSet(frame, varFrom(7, INT), varFrom(state->stack, DICT));
            state->stack = frame;

            for(int j = 0; j < params->length; j++){
                Var key = *arrayGet(params, j);
                Var value = *arrayGet(args, j);
                dictSet(frame, key, value);
            }

            Array* code = f->code;
            for(int j = 0; j < code->length; j++){
                result = eval(state, *arrayGet(code, j));

                if(result.type == ERROR){
                    printf("(line %d) While executing function\n", (int)f->lineNo);
                    return result;
                }
            }

            state->stack = dictGet(state->stack, varFrom(7, INT))->ptr;
        }
        else {
            printf("(line %d) Object is not callable\n", (int)c->lineNo);
            return varFrom(0, ERROR);
        }
        arrayPop(state->tmp);
    }
    return result;
}

int compare(Var a, Var b){
    if(a.type != b.type)
        return a.type - b.type;
    switch(a.type){
    case FUNCTION:
    case BUILTIN:
    case INT:     return a._int - b._int;
    case FLOAT:   return a._double - b._double < 0 ? -1 : (a._double - b._double > 0 ? 1 : 0);
    case ARRAY:   return arrayCompare(a.ptr, b.ptr);
    case IDENTIFIER:
    case STRING:  return stringCompare(a.ptr, b.ptr);
    case DICT:    return dictCompare(a.ptr, b.ptr);
    default:      return 0;
    }
}
size_t toHash(Var v){
    switch(v.type){
    case INT:
    case FLOAT:
    case FUNCTION:
    case BUILTIN:    return v._int;
    case ARRAY:      return arrayToHash(v.ptr);
    case STRING:
    case IDENTIFIER: return stringToHash(v.ptr);
    case DICT:       return dictToHash(v.ptr);
    default: return 0;
    }
}
void toString(Var v, void* out, size_t indent){
    switch(v.type){
    case   IDENTIFIER:
    case       STRING: return stringToString(v.ptr, out, indent);
    case        ARRAY: return arrayToString(v.ptr, out, indent);
    case         DICT: return dictToString(v.ptr, out, indent);
    case     FUNCTION: return functionToString(v.ptr, out, indent);
    case      BUILTIN: return builtinToString(out, indent);
    case          INT: return intToString(v._int, out, indent);
    case        FLOAT: return floatToString(v._double, out, indent);
    default          : return;
    }
}
Var eval(GlobalState* state, Var v){
    switch(v.type){
        case CALL:       return evalCall(state, v.ptr);
        case ASSIGNMENT: return evalAssign(state, v.ptr);
        case IDENTIFIER: return evalIdentifier(state, v.ptr);
        case ARRAY:      return varFrom(arrayEval(state, v.ptr), ARRAY);
        case DICTEXPR:   return varFrom(dictEval(state, v.ptr), DICT);
        case STRING:     return varFrom(stringDup(state, v.ptr), STRING);
        default:         return v;
    }
}

void evaluate(GlobalState* state){
    if(!state || !state->stack || !state->module)
        return;

    Array* expressions = state->module;
    for(int i = 0; i < expressions->length; i++)
        eval(state, *arrayGet(expressions, i));

    unmark(state);
    sweep(state);
}
