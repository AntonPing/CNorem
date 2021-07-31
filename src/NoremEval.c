#include "Norem.h"


#define STACK_SIZE 32

Term_t FRAME;
Term_t ROOT;
Term_t HOLE;

typedef struct Task_t {
    Term_t* *stack;
    Term_t* *sp;
    Term_t* with;
    Term_t* ret;
} Task_t;

Task_t* new_task(Term_t* with) {
    Task_t* task = malloc(sizeof(Task_t));
    
    task->with = with;




}


Term_t* eval(Term_t* term) {
    Term_t* stack[16];
    Term_t** sp = &stack[0];
    Term_t* with = term;

    assert(with != NULL);

    #ifdef DEBUG
        printf("eval call: ");
        show_term(with);
        printf("\n");
    #endif

    eval_loop:
    #ifdef DEBUG
        //show_term(with);
        //printf("\n");
        SHOW_STACK();
    #endif
    
    switch(with->tag) {
        Term_t *x,*y,*z;
        case APP:
            PUSH(with->t2);
            NEXT(with->t1);
        case I:
            ARG_1(x);
            NEXT(x);
        case K:
            ARG_2(x,y);
            NEXT(x);
        case S:
            ARG_3(x,y,z);
            PUSH(new_app(y,z));
            PUSH(z);
            NEXT(x);
        
        #define BINOP(assert1,assert2,result) do { \
            ARG_2(x,y); \
            EVAL(x); \
            EVAL(y); \
            assert(assert1); \
            assert(assert2); \
            NEXT(result); \
        } while(0)

        case ADDI: BINOP(
            x->tag == INT,
            y->tag == INT,
            new_int(x->int_v + y->int_v)
        );
        case SUBI: BINOP(
            x->tag == INT,
            y->tag == INT,
            new_int(x->int_v - y->int_v)
        );
        case MULI: BINOP(
            x->tag == INT,
            y->tag == INT,
            new_int(x->int_v * y->int_v)
        );
        case DIVI: BINOP(
            x->tag == INT,
            y->tag == INT,
            new_int(x->int_v / y->int_v)
        );
        case EQL: BINOP(
            x->tag == INT,
            y->tag == INT,
            new_bool(x->int_v == y->int_v)
        );
        case GRT: BINOP(
            x->tag == INT,
            y->tag == INT,
            new_bool(x->int_v > y->int_v)
        );
        case LSS: BINOP(
            x->tag == INT,
            y->tag == INT,
            new_bool(x->int_v < y->int_v)
        );
        #undef BINOP

        #define UNIOP(assert1,result) do { \
            ARG_1(x); \
            EVAL(x); \
            assert(assert1); \
            NEXT(result); \
        } while(0)

        case NEGI: UNIOP(
            x->tag == INT,
            new_int(x->int_v * -1)
        );
        case NOT: UNIOP(
            x->tag == BOOL,
            new_bool(!x->bool_v)
        );

        #undef UNIOP
        
        case IF:
            ARG_3(x,y,z);
            EVAL(x);
            assert(x->tag == BOOL);
            NEXT(x->bool_v ? y : z);
        
        /*
        case PRINTI:
            POP_2(x,y); EVAL(x);
            assert(x->tag == INT);
            printf("printi %ld\n", x->int_v);
            NEXT(y);
        case EXIT:
            exit(0);
        */

        case SYMB: {
            Dict_t* dict = dict_get(with->symb_v);
            if(dict != NULL) {
                assert(dict->compiled != NULL);
                LOG("dynamic linking %s ...\n",with->symb_v);
                //show_term(dict->compiled);
                //printf("\nHERE\n");
                //LOG("dynamic linking!\n");
                NEXT(dict->compiled);
            } else {
                PANIC("undefined symbol!\n");
            }
        }
        case INT:
        case REAL:
        case CHAR:
        case BOOL:
            if(sp == &stack[0]) {
                #ifdef DEBUG
                    printf("return value: ");
                    show_term(with);
                    printf("\n");
                #endif
                // RETURN PART
                


                return with;
            } else {
                SHOW_STACK();
                PANIC("basic data can't be function!\n");
            }
        default:
            PANIC("Unknown tag when eval term!\n");
    }
}
