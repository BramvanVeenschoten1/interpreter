#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "array.h"
#include "dict.h"
#include "eval.h"
#include "gc.h"
#include "mystring.h"
#include "var.h"

static Var print(GlobalState* state, Array* args){
    if(!args->length)
        return varFrom(0, NONE);
    String* s = new(state, STRING);
    for(int i = 0; i < args->length; i++){
        toString(*arrayGet(args, i), s, 0);
        quote(s);
        s->length = 0;
    }
    return varFrom(0, NONE);
}
static Var _true(GlobalState* state, Array* args){
    if(args->length != 2)
        return varFrom(0, ERROR);
    return *arrayGet(args, 0);
}
static Var _false(GlobalState* state, Array* args){
    if(args->length != 2)
        return varFrom(0, ERROR);
    return *arrayGet(args, 1);
}
static Var less(GlobalState* state, Array* args){
    if(args->length != 2)
        return varFrom(0, ERROR);
    if(compare(*arrayGet(args, 0), *arrayGet(args, 1)) < 0)
        return varFrom(&_true, BUILTIN);
    return varFrom(&_false, BUILTIN);
}
static Var equals(GlobalState* state, Array* args){
    if(args->length != 2)
        return varFrom(0, ERROR);
    if(compare(*arrayGet(args, 0), *arrayGet(args, 1)) == 0)
        return varFrom(&_true, BUILTIN);
    return varFrom(&_false, BUILTIN);
}
static Var neg(GlobalState* state, Array* args){
    if(args->length != 1)
        return varFrom(0, ERROR);
    Var result = *arrayGet(args, 0);
    if(result.type == INT)
        return (Var){{result._int * -1}, INT};
    if(result.type == FLOAT)
        return (Var){{result._double * -1}, FLOAT};
    if(result.ptr == _true)
        return varFrom(&_false, BUILTIN);
    if(result.ptr == _false)
        return varFrom(&_true, BUILTIN);
    return (Var){{0}, ERROR};
}

static Var add(GlobalState* state, Array* args){
    if(args->length < 1)
        return varFrom(0, ERROR);
    Var result = *arrayGet(args, 0);
    switch(result.type){
        case INT:
            for(int i = 1; i < args->length; i++){
                Var rhs = *arrayGet(args, i);
                if(rhs.type == INT)
                    result._int += rhs._int;
                else if(rhs.type == FLOAT)
                    result._int += rhs._double;
                else
                    return varFrom(0, ERROR);
            }
            return result;
        case FLOAT:
            for(int i = 1; i < args->length; i++){
                Var rhs = *arrayGet(args, i);
                if(rhs.type == INT)
                    result._double += rhs._int;
                if(rhs.type == FLOAT)
                    result._double += rhs._double;
                else
                    return varFrom(0, ERROR);
            }
            return result;
        case STRING:
            for(int i = 1; i < args->length; i++){
                Var rhs = *arrayGet(args, i);
                toString(rhs, result.ptr, 0);
            }
            return result;
        case ARRAY:
            for(int i = 1; i < args->length; i++){
                Var rhs = *arrayGet(args, i);
                if(rhs.type == ARRAY)
                    arrayAppend(result.ptr, rhs.ptr);
                else
                    arrayPush(result.ptr, rhs);
            }
            return result;
        default: return varFrom(0, ERROR);
    }
}
static Var mul(GlobalState* state, Array* args){
    if(args->length < 1)
        return varFrom(0, ERROR);
    Var result = *arrayGet(args, 0);
    if(result.type == INT){
        for(int i = 1; i < args->length; i++){
            Var rhs = *arrayGet(args, i);
            if(rhs.type == INT)
                result._int *= rhs._int;
            else if(rhs.type == FLOAT)
                result._int *= rhs._double;
            else
                return varFrom(0, ERROR);
        }
    }
    else if(result.type == FLOAT){
        for(int i = 1; i < args->length; i++){
            Var rhs = *arrayGet(args, i);
            if(rhs.type == INT)
                result._double *= rhs._int;
            if(rhs.type == FLOAT)
                result._double *= rhs._double;
            else
                return varFrom(0, ERROR);
        }
    }
    return result;
}

static Var sub(GlobalState* state, Array* args){
    if(args->length != 2)
        return varFrom(0, ERROR);
    Var lhs = *arrayGet(args, 0);
    Var rhs = *arrayGet(args, 1);
    if(lhs.type == INT && rhs.type == INT)
        return (Var){{lhs._int - rhs._int}, INT};
    if(lhs.type == INT && rhs.type == INT)
        return (Var){{(long long)(lhs._int - rhs._double)}, INT};
    if(lhs.type == FLOAT && rhs.type == INT)
        return (Var){{(double)(lhs._double - rhs._int)}, FLOAT};
    if(lhs.type == FLOAT && rhs.type == INT)
        return (Var){{lhs._double - rhs._double}, FLOAT};
    return varFrom(0, ERROR);
}
static Var _div(GlobalState* state, Array* args){
    if(args->length != 2)
        return varFrom(0, ERROR);
    Var lhs = *arrayGet(args, 0);
    Var rhs = *arrayGet(args, 1);
    if(lhs.type == INT && rhs.type == INT)
        return (Var){{lhs._int / rhs._int}, INT};
    if(lhs.type == INT && rhs.type == INT)
        return (Var){{(long long)(lhs._int / rhs._double)}, INT};
    if(lhs.type == FLOAT && rhs.type == INT)
        return (Var){{(double)(lhs._double / rhs._int)}, FLOAT};
    if(lhs.type == FLOAT && rhs.type == INT)
        return (Var){{lhs._double / rhs._double}, FLOAT};
    return varFrom(0, ERROR);
}
static Var mod(GlobalState* state, Array* args){
    if(args->length != 2)
        return varFrom(0, ERROR);
    Var lhs = *arrayGet(args, 0);
    Var rhs = *arrayGet(args, 1);
    if(lhs.type == INT && rhs.type == INT)
        return varFrom(lhs._int % rhs._int, INT);
    return varFrom(0, ERROR);
}

static Var set(GlobalState* state, Array* args){
    if(args->length != 3)
        return varFrom(0, ERROR);
    Var result = *arrayGet(args, 0);
    Var key = *arrayGet(args, 1);
    Var value = *arrayGet(args, 2);
    if(result.type == DICT){
        dictSet(result.ptr, key, value);
        return result;
    }
    if(result.type != ARRAY || key.type != INT || ((Array*)result.ptr)->length <= key._int)
        return varFrom(0, ERROR);
    *arrayGet(result.ptr, key._int) = value;
    return result;
}
static Var get(GlobalState* state, Array* args){
    if(args->length != 2)
        return varFrom(0, ERROR);
    Var item = *arrayGet(args, 0);
    Var key = *arrayGet(args, 1);
    Var* result;
    if(item.type == DICT){
        result = dictGet(item.ptr, key);
        if(result)
            return *result;
        return varFrom(0, ERROR);
    }
    if(item.type != ARRAY || key.type != INT || ((Array*)item.ptr)->length <= key._int)
        return varFrom(0, ERROR);
    return *arrayGet(item.ptr, key._int);
}
static Var pop(GlobalState* state, Array* args){
    if(args->length < 1)
        return varFrom(0, ERROR);
    Var result = *arrayGet(args, 0);
    if(result.type == ARRAY){
        arrayPop(result.ptr);
        return result;
    }
    if(result.type != DICT || args->length != 2)
        return varFrom(0, ERROR);
    Var key = *arrayGet(args, 1);
    dictRemove(result.ptr, key);
    return result;
}
static Var length(GlobalState* state, Array* args){
    if(args->length != 1)
        return varFrom(0, ERROR);
    Var result = *arrayGet(args, 0);
    switch(result.type){
        case DICT: return (Var){{(size_t)(((Dict*)result.ptr)->count)}, INT};
        case ARRAY: return (Var){{(size_t)(((Array*)result.ptr)->length)}, INT};
        case STRING: return (Var){{(size_t)(((String*)result.ptr)->length)}, INT};
        default: return varFrom(0, ERROR);
    }
}

void stateInit(GlobalState* state){
    state->stack = new(state, DICT);
    state->tmp = new(state, ARRAY);

    dictSet(state->stack, varFrom(identifierFromChar(state, "print"),  IDENTIFIER), varFrom(&print,  BUILTIN) );
    dictSet(state->stack, varFrom(identifierFromChar(state, "true"),   IDENTIFIER), varFrom(&_true,  BUILTIN) );
    dictSet(state->stack, varFrom(identifierFromChar(state, "false"),  IDENTIFIER), varFrom(&_false, BUILTIN) );
    dictSet(state->stack, varFrom(identifierFromChar(state, "less"),   IDENTIFIER), varFrom(&less,   BUILTIN) );
    dictSet(state->stack, varFrom(identifierFromChar(state, "equals"), IDENTIFIER), varFrom(&equals, BUILTIN) );
    dictSet(state->stack, varFrom(identifierFromChar(state, "neg"),    IDENTIFIER), varFrom(&neg,    BUILTIN) );

    dictSet(state->stack, varFrom(identifierFromChar(state, "add"),    IDENTIFIER), varFrom(&add,    BUILTIN) );
    dictSet(state->stack, varFrom(identifierFromChar(state, "mul"),    IDENTIFIER), varFrom(&mul,    BUILTIN) );

    dictSet(state->stack, varFrom(identifierFromChar(state, "sub"),    IDENTIFIER), varFrom(&sub,    BUILTIN) );
    dictSet(state->stack, varFrom(identifierFromChar(state, "div"),    IDENTIFIER), varFrom(&_div,   BUILTIN) );
    dictSet(state->stack, varFrom(identifierFromChar(state, "mod"),    IDENTIFIER), varFrom(&mod,    BUILTIN) );

    dictSet(state->stack, varFrom(identifierFromChar(state, "set"),    IDENTIFIER), varFrom(&set,    BUILTIN) );
    dictSet(state->stack, varFrom(identifierFromChar(state, "get"),    IDENTIFIER), varFrom(&get,    BUILTIN) );
    dictSet(state->stack, varFrom(identifierFromChar(state, "pop"),    IDENTIFIER), varFrom(&pop,    BUILTIN) );
    dictSet(state->stack, varFrom(identifierFromChar(state, "length"), IDENTIFIER), varFrom(&length, BUILTIN) );
}
