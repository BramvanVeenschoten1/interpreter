#include "var.h"
#include "eval.h"
#include "gc.h"

int main(int argc, char** argv)
{
    GlobalState state = {0};

    stateInit(&state);
    parseModule(&state, "main.lol");
    evaluate(&state);
    return 0;
}
