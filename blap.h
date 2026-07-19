//
//  #      ""#                         #
//  #mmm     #     mmm   mmmm          # mm
//  #" "#    #    "   #  #" "#         #"  #
//  #   #    #    m"""#  #   #         #   #
//  ##m#"    "mm  "mm"#  ##m#"    #    #   #
//                       #
//                       "

#ifndef BLAP_H
#define BLAP_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef _WIN32
#define blap_string char *
#else // TODO: just in case it's different
#define blap_string char *
#endif

typedef struct {
    int count;
    int capacity;
    blap_string *args;
} cmd_t;

void cmd_append(cmd_t *p, const char *command);
void cmd_run(cmd_t p);
void cmd_close(cmd_t *p);
void cmd_dump(cmd_t p, const char * tag);

#ifdef CMD_MACROS

#define ARG(p, cmd) cmd_append(&(p), (cmd))

#endif

#ifdef CMD_IMPLEMENTATION

#define INIT_CAPACITY 16

void __expect(bool p, const char *s);
blap_string __blapstr(const char* s);
void __whitespace(int n);

blap_string __blapstr(const char* s)
{
    size_t n = strlen(s);
    blap_string out = (blap_string)malloc(n+1);
    strncpy(out, s, n);
    out[n] = '\0';
    return out;
}

void __expect(bool p, const char *s)
{
    if (p) return;
    fprintf(stderr, "%s\n", s);
}

void cmd_append(cmd_t *p, const char *command)
{
    if (p->count >= p->capacity || p->capacity == 0) {
        p->capacity = !p->capacity ? INIT_CAPACITY:p->capacity * 2;
        p->args = (blap_string *)realloc(p->args, p->capacity * sizeof(blap_string));
        __expect(p->args, "Download free ram at goonzone.com");
    }
    p->args[p->count] = __blapstr(command);
    ++p->count;
}

void cmd_close(cmd_t *p)
{
    for (int i = 0; i < p->count; ++i) {
        free(p->args[i]);
    }
    free(p->args);
    memset(p, 0, sizeof(cmd_t));
}

#define MAX_CMD_SIZE 65535


void cmd_run(cmd_t p)
{
    char command[MAX_CMD_SIZE] = {0};
    size_t j = 0;

    for (int i = 0; i < p.count; ++i) {
        for (char *c = p.args[i]; *c; ++c, ++j)
            command[j] = *c;
        command[j++] = ' ';
    }
    system(command);
}

void __whitespace(int n)
{
    for (int i = 1; i <= n; ++i)
        putchar(' ');
}

void cmd_dump(cmd_t p, const char * tag)
{
    printf("%s: \n", tag);
    __whitespace(2);
    for (int i = 0; i < p.count; ++i)
        printf("%s ", p.args[i]);
    printf("\n");
}

#endif

#endif // BLAP_H
