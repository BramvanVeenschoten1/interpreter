#ifndef VAR_H_INCLUDED
#define VAR_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>

#define varFrom(a, b) (Var){{(size_t)(a)}, (b)}

typedef enum Type Type;

enum Type {
    // Primitives
    NONE = 0,
    INT,
    FLOAT,
    BUILTIN,
    ERROR,
    // Objects
    STRING,
    ARRAY,
    DICT,
    FUNCTION,
    // Expressions
    DICTEXPR,
    ASSIGNMENT,
    IDENTIFIER,
    CALL,
};

typedef struct Array Array;
typedef struct Assign Assign;
typedef struct Block Block;
typedef struct Call Call;
typedef struct Dict Dict;
typedef struct Function Function;
typedef struct GlobalState GlobalState;
typedef struct Pair Pair;
typedef struct String String;
typedef struct Var Var;

struct Var{
    union{
        long long _int;
        double _double;
        void* ptr;
    };
    Type type;
};
struct Array {
    Block* next;
    char type;
    char tag;
    size_t length;
    size_t capacity;
    Var* data;
};
struct Assign{
    Block* next;
    char type;
    char tag;
    size_t lineNo;
    String* id;
    Var expr;
};
struct Block {
    Block* next;
    char type;
    char tag;
};
struct Call{
    Block* next;
    char type;
    char tag;
    size_t lineNo;
    Var expr;
    Array* argLists;
};
struct Dict {
    Block* next;
    char type;
    char tag;
    size_t length;
    size_t count;
    Pair* data;
};
struct Function{
    Block* next;
    char type;
    char tag;
    size_t lineNo;
    Array* params;
    Array* code;
};
struct GlobalState{
    Dict* stack;
    Array* module;
    Block* gc;
    Array* tmp;
    int newCount;
};
struct Pair {
    Var key;
    Var value;
};
struct String {
    Block* next;
    char type;
    char tag;
    size_t length;
    size_t capacity;
    char* data;
};

#endif // VAR_H_INCLUDED
