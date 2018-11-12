#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "array.h"
#include "dict.h"
#include "eval.h"
#include "gc.h"
#include "mystring.h"
#include "var.h"

typedef struct {
    String* source;
    char* input;
    char* end;
    int lineNo;
} ParserState;

static Var parseExpression(GlobalState*, ParserState*);
static Var parseArray(GlobalState*, ParserState*, char, char, char);

static void parseWhitespace(ParserState* state){
    char c = *(state->input);
    while(c == ' ' || c == '\t' || c == '\r' || c == '\n'){
        if(state->input >= state->end)
            return;
        if(c == '\n')
            state->lineNo++;
        state->input++;
        c = *(state->input);
    }
}
static char parseRange(ParserState* state, char down, char up){
    if(state->input == state->end)
        return 0;
    char c = *(state->input);
    if(c >= down && c <= up){
        if(c == '\n')
            state->lineNo++;
        state->input++;
        return 1;
    }
    return 0;
}
static char parseChar(ParserState* state, char x){
    if(state->input == state->end)
        return 0;
    char c = *(state->input);
    if(c == x){
        if(x == '\n')
            state->lineNo++;
        state->input++;
        return 1;
    }
    return 0;
}
static char parseEscape(ParserState* state){
    char c = *(state->input);
    state->input++;
    switch(c){
    case '"':  return '"';
    case '\\': return '\\';
    case 'n':  return '\n';
    case 't':  return '\t';
    case 'r':  return '\r';
    default:
        {
            char result = 0;
            if(c >= 'a' && c <= 'z')
                result = c - 'a' + 10;
            else if(c >= 'A' && c <= 'Z')
                result = c - 'A' + 10;
            else if(c >= '0' && c <= '9')
                result = c - '0';
            else return 0;
            c = *(state->input);
            state->input++;
            result *= 16;
            if(c >= 'a' && c <= 'z')
                result = c - 'a' + 10;
            else if(c >= 'A' && c <= 'Z')
                result = c - 'A' + 10;
            else if(c >= '0' && c <= '9')
                result = c - '0';
            else return 0;
            return result;
        }
    }
}
static char parseEnd(ParserState* state){
    return state->input >= state->end;
}

static Var parseNumber(ParserState* state){
    state->input--;
    char* dptr;
    char* lptr;
    double d = strtod(state->input, &dptr);
    long long l = strtol(state->input, &lptr, 10);
    if((size_t)dptr > (size_t)lptr){
        state->input = dptr;
        Var result;
        result.type = FLOAT;
        result._double = d;
        return result;
    }
    state->input = lptr;
    Var result;
    result.type = INT;
    result._int = l;
    return result;
}
static Var parseString(GlobalState* gState, ParserState* pState){
    size_t length = 0;
    ParserState save = *pState;
    while(1){
        if(parseEnd(&save)){
            printf("(line %d) While parsing string: Unexpected end of input\n", save.lineNo);
            return varFrom(0, ERROR);
        }
        if(save.input[0] == '"')
            break;
        if(save.input[0] == '\\')
            parseEscape(&save);
        if(save.input[0] == '\n')
            save.lineNo++;
        save.input++;
        length++;
    }
    String* s = new(gState, STRING);
    s->capacity = length;
    s->data = _malloc(length);

    length = 0;
    while(1){
        if(*(pState->input) == '"'){
            pState->input++;
            break;
        }
        else if(*(pState->input) == '\\'){
            pState->input++;
            s->data[length] = parseEscape(pState);
        }
        else {
            if(*(pState->input) == '\n')
                pState->lineNo++;
            s->data[length] = *(pState->input);
            pState->input++;
        }
        length++;
    }
    s->length = length;
    return varFrom(s, STRING);
}
static Var parseIdentifier(GlobalState* gState, ParserState* pState){
    char* begin = pState->input;
    size_t length = 0;
    while(1){
        char c = *(pState->input);
        if(c == '\n'){pState->lineNo++; break;}
        if( c == ':' ||
            c == '=' ||
            c == ';' ||
            c == ',' ||
            c == '(' ||
            c == ')' ||
            c == '[' ||
            c == ']' ||
            c == '{' ||
            c == '}' ||
            c == ' ' ||
            c == '\t'||
            c == '\r') break;
        pState->input++;
        length++;
    }
    if(!length){
        printf("(line %d) While parsing identifier: Unexpected \'%c\'\n", pState->lineNo, *pState->input);
        return varFrom(0, ERROR);
    }
    String* s = new(gState, STRING);
    s->data = _malloc(length);
    s->capacity = length;
    s->length = length;
    memcpy(s->data, begin, length);
    return varFrom(s, IDENTIFIER);
}

static Var parseArray(GlobalState* gState, ParserState* pState, char delim, char end, char isParams){
    Array* a = new(gState, ARRAY);
    arrayPush(gState->tmp, varFrom(a, ARRAY));
    while(1){
        parseWhitespace(pState);
        if(parseChar(pState, end)){
            arrayPop(gState->tmp);
            return varFrom(a, ARRAY);
        }

        Var expr = parseExpression(gState, pState);
        if(expr.type == ERROR){
            arrayPop(gState->tmp);
            return expr;
        }

        if(isParams && expr.type != IDENTIFIER){
            printf("(line %d) While parsing Params: only identifiers allowed\n", pState->lineNo);
            arrayPop(gState->tmp);
            return varFrom(0, ERROR);
        }

        arrayPush(a, expr);

        parseWhitespace(pState);
        if(parseChar(pState, delim)) continue;
        if(parseChar(pState, end)){
            arrayPop(gState->tmp);
            return varFrom(a, ARRAY);
        }

        printf("(line %d) While parsing array: Expected \'%c\' or \'%c\'\n", pState->lineNo, delim, end);
        arrayPop(gState->tmp);
        return varFrom(0, ERROR);
    }
}
static Var parseDict(GlobalState* gState, ParserState* pState){
    Array* a = new(gState, ARRAY);
    arrayPush(gState->tmp, varFrom(a, ARRAY));
    while(1){
        parseWhitespace(pState);
        if(parseChar(pState, '}')){
            arrayPop(gState->tmp);
            return varFrom(a, DICTEXPR);
        }

        Var expr = parseExpression(gState, pState);
        if(expr.type == ERROR){
            arrayPop(gState->tmp);
            return expr;
        }

        arrayPush(a, expr);
        parseWhitespace(pState);
        if(!parseChar(pState, ':')){
            printf("(line %d) While parsing dict: Expected \':\' after key expression\n", pState->lineNo);
            arrayPop(gState->tmp);
            return varFrom(0, ERROR);
        }
        expr = parseExpression(gState, pState);
        if(expr.type == ERROR){
            arrayPop(gState->tmp);
            return expr;
        }

        arrayPush(a, expr);

        parseWhitespace(pState);
        if(parseChar(pState, ',')) continue;
        if(parseChar(pState, '}')){
            arrayPop(gState->tmp);
            return varFrom(a, DICTEXPR);
        }

        printf("(line %d) While parsing dict: expected \',\' or \'}\'\n", pState->lineNo);
        arrayPop(gState->tmp);
        return varFrom(0, ERROR);
    }
}
static Var parseCall(GlobalState* gState, ParserState* pState, Var func, size_t lineNo){
    Array* argLists = new(gState, ARRAY);
    arrayPush(gState->tmp, varFrom(argLists, ARRAY));
    while(1){
        Var result = parseArray(gState, pState, ',', ')', 0);
        if(result.type == ERROR){
            arrayPop(gState->tmp);
            return result;
        }

        arrayPush(argLists, result);

        parseWhitespace(pState);
        if(!parseChar(pState, '(')){
            Call* c = new(gState, CALL);
            c->lineNo = lineNo;
            c->expr = func;
            c->argLists = argLists;
            arrayPop(gState->tmp);
            return varFrom(c, CALL);
        }
    }
}
static Var parseAssign(GlobalState* gState, ParserState* pState){
    size_t lineNo = pState->lineNo;
    Var id = parseIdentifier(gState, pState);
    if(id.type == ERROR)
        return id;

    parseWhitespace(pState);
    if(!parseChar(pState, '='))
        return id;

    arrayPush(gState->tmp, id);

    Var expr = parseExpression(gState, pState);

    if(expr.type == ERROR){
        arrayPop(gState->tmp);
        return expr;
    }

    arrayPush(gState->tmp, expr);

    Assign* a = new(gState, ASSIGNMENT);
    arrayPop(gState->tmp);
    arrayPop(gState->tmp);

    a->lineNo = lineNo;
    a->expr = expr;
    a->id = id.ptr;
    return varFrom(a, ASSIGNMENT);
}
static Var parseFunction(GlobalState* gState, ParserState* pState){
    size_t lineNo = pState->lineNo;

    Var params = parseArray(gState, pState, ',', ')', 1);
    if(params.type == ERROR)
        return params;

    parseWhitespace(pState);
    if(!parseChar(pState, '{')){
        printf("(line %d) While parsing function: expected block after params\n", pState->lineNo);
        return varFrom(0, ERROR);
    }

    arrayPush(gState->tmp, params);

    Var block = parseArray(gState, pState, ';', '}', 0);
    if(block.type == ERROR){
        arrayPop(gState->tmp);
        return block;
    }

    arrayPush(gState->tmp, block);
    Function* f = new(gState, FUNCTION);
    arrayPop(gState->tmp);
    arrayPop(gState->tmp);

    f->lineNo = lineNo;
    f->code = block.ptr;
    f->params = params.ptr;
    return varFrom(f, FUNCTION);
}
static Var parseLiteralCall(GlobalState* gState, ParserState* pState){
    Var func = parseFunction(gState, pState);
    if(func.type == ERROR)
        return func;

    parseWhitespace(pState);
    size_t lineNo = pState->lineNo;
    if(!parseChar(pState, '('))
        return func;

    arrayPush(gState->tmp, func);
    Var result = parseCall(gState, pState, func, lineNo);
    arrayPop(gState->tmp);
    return result;

}
static Var parseFunctionCall(GlobalState* gState, ParserState* pState){
    Var func = parseAssign(gState, pState);
    if(func.type != IDENTIFIER)
        return func;

    parseWhitespace(pState);
    size_t lineNo = pState->lineNo;
    if(!parseChar(pState, '('))
        return func;

    arrayPush(gState->tmp, func);
    Var result = parseCall(gState, pState, func, lineNo);
    arrayPop(gState->tmp);
    return result;
}
static Var parseExpression(GlobalState* gState, ParserState* pState){
    parseWhitespace(pState);
    if(parseRange(pState, '0', '9')) return parseNumber(pState);
    if(parseChar(pState, '"'))       return parseString(gState, pState);
    if(parseChar(pState, '['))       return parseArray(gState, pState, ',', ']', 0);
    if(parseChar(pState, '{'))       return parseDict(gState, pState);
    if(parseChar(pState, '('))       return parseLiteralCall(gState, pState);
    return parseFunctionCall(gState, pState);
}

void parseModule(GlobalState* gState, char* fileName){
    FILE* f = fopen(fileName, "r");
    if(!f){
        printf("Couldn't open file: %s\n", fileName);
        return;
    }

    String* source = stringFromFile(gState, f);
    arrayPush(gState->tmp, varFrom(source, STRING));
    fclose(f);

    ParserState pState;
    pState.source = source;
    pState.input = source->data;
    pState.end = source->data + source->length;
    pState.lineNo = 1;

    gState->module = new(gState, ARRAY);

    while(1){
        Var expr = parseExpression(gState, &pState);
        if(expr.type == ERROR){
            gState->module = NULL;
            break;
        }
        arrayPush(gState->module, expr);

        parseWhitespace(&pState);
        if(!parseChar(&pState, ';')){
            printf("(line %d) While parsing module: Expected \';\' after expression\n", pState.lineNo);
            gState->module = NULL;
            break;
        }
        parseWhitespace(&pState);
        if(parseEnd(&pState))
            break;
    }
    arrayPop(gState->tmp);
}
