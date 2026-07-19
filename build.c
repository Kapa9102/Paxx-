#define CMD_IMPLEMENTATION
#include "blap.h"

#ifdef _WIN32
    #define CC "clang"
#else
    #define CC "gcc"
#endif

#define MAIN   "main.c"
#define OUTPUT "main"

void build_main(cmd_t *p);

int main()
{
    cmd_t main = {0};
    fprintf(stderr, "[1] INFO : Building %s ... \n", MAIN);
    build_main(&main);
    cmd_close(&main);
}

void build_main(cmd_t *p)
{
    cmd_append(p, CC);
    cmd_append(p, MAIN);
    cmd_append(p, "-o");
    cmd_append(p, OUTPUT);
    cmd_append(p, "-O3 -lm -lraylib -lpthread -ldl -lxcb -lrt -lX11");
    cmd_append(p, "-I include -L lib");
    cmd_append(p, "-Wl,-rpath=$(pwd)/lib");
    cmd_run(*p);
}
