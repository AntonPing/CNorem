#ifndef NOREM_H
#define NOREM_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdatomic.h>
#include <unistd.h>
#include <pthread.h>
#include <setjmp.h>
#include <assert.h>
#include <ctype.h>
#include <uchar.h>

typedef int64_t int_t;
typedef double real_t;
typedef char char_t;
typedef bool bool_t;
typedef char* symb_t;
typedef char* string_t;

#ifdef DEBUG
#define DBG(...) do{ \
    fprintf(stderr, "[DEBUG]%s %s(Line %d): ",__FILE__,__FUNCTION__,__LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    } while(0)
#else
#define DBG(...)
#endif

#define PANIC(...) do{ \
    fprintf(stderr, "[PANIC]%s %s(Line %d): ",__FILE__,__FUNCTION__,__LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    exit(1); \
} while(0)


typedef enum Tag_t {
    INT=0, REAL, CHAR, BOOL, SYMB,
    LAMB, FUNC, CONS, TERM, STR,

    I=100,K,S,B,C,SP,BS,CP,Y,
    ADDI,
    //ALLOC,FREE,
    //READI,WRITEI
    PRINTI,EXIT,
    DEFINE,
    NIL,
} Tag_t;

struct Term_t;

typedef struct Lambda_t {
    symb_t x;
    struct Term_t* t;
} Lambda_t;

typedef struct Term_t {
    union {
        struct Term_t* tag;
        struct Term_t* t1;
    };
    union {
        int_t int_v;
        real_t real_v;
        char_t char_v;
        bool_t bool_v;
        symb_t symb_v;
        struct Lambda_t* lamb_v;
        struct Term_t* t2;
    };
} Term_t;

typedef struct Dict_t {
    symb_t name;
    Term_t* raw;
    Term_t* compiled;
    Term_t* linked;
    string_t text;
    struct Dict_t* next;
} Dict_t;

// Norem.c
extern Term_t tags[256];
Dict_t* dict_get(symb_t key);

// NoremShow.c
void show_term(Term_t* term);
void show_lamb(Term_t* term);
void show_dict(Dict_t* dict);

// NoremCompile.c
bool is_tag(Term_t* term);
bool is_singleton(Term_t* term);
bool is_atom(Term_t* term);
bool is_app(Term_t* term);
bool is_var(Term_t* term);
bool is_lambda(Term_t* term);
Term_t* term_compile(Term_t* term);
Term_t* term_link(Term_t* term);

// NoremHeap.c
Term_t* new_app(Term_t* t1, Term_t* t2);
Term_t* new_cons(Term_t* t1, Term_t* t2);
Term_t* new_int(int_t value);
Term_t* new_real(real_t value);
Term_t* new_char(char_t value);
Term_t* new_bool(bool_t value);
Term_t* new_symb(symb_t value);
Term_t* new_lamb(symb_t x, Term_t* t);

// NoremParse.c
bool term_parse(char_t* str, Term_t** ret);
bool definition(char_t* str, symb_t* key, Term_t** value);

// NoremEval.c
Term_t* eval(Term_t* term);

// NoremSymb.c
symb_t to_symb(char_t* str);
string_t substr(string_t str, size_t n);
string_t slice(char_t* start, char_t* end);

/*
Bind_t* dict_get_bind(symb_t key);
Bind_t* dict_new_bind(symb_t key);
void bind_set_text(Bind_t* bind, string_t text);
void bind_update(Bind_t* bind);
*/

#endif
