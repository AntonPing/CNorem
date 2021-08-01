#include "Norem.h"

#define STACK_SIZE 32

Term_t FRAME;
Term_t HOLE;


Task_t* new_task(Term_t* with) {
    Task_t* task = malloc(sizeof(Task_t));
    Term_t* *stack = malloc(sizeof(Term_t*) * STACK_SIZE);
    stack[0] = &HOLE;
    stack[1] = &sing[EXIT];
    stack[2] = &FRAME;
    stack[3] = with;
    task->stack_base = stack;
    task->stack_ceil = &stack[STACK_SIZE - 1];
    task->sp = &stack[2];
    task->with = with;
    return task;
}

#define PUSH(reg) \
    sp ++; \
    assert(sp <= stack_ceil); \
    *sp = reg

#define ARG_1 sp[0]
#define ARG_2 sp[-1]
#define ARG_3 sp[-2]
#define ARG_4 sp[-3]

#define POP(n) \
    sp -= n

#define WITH(x) \
    with = x

#define NEXT(clk) \
    step -= clk; \
    continue

#define CALL(x) \
    x = &HOLE; \
    PUSH(with); \
    PUSH(&FRAME); \
    WITH(x); \
    NEXT(2)


#define RESERVE(n) \
    for(int i=0; i<n; i++) \
        assert(sp[-i] != &FRAME)


Term_t* eval(Task_t* task, int_t timeslice) {
    // load the task
    Task_t* task_return = task;
    Term_t* *stack_base = task->stack_base;
    Term_t* *stack_ceil = task->stack_ceil;
    Term_t* *sp = task->sp;
    Term_t* with = *sp--; // pop top as with
    

    // run timeslice
    assert(timeslice >= 0);
    int_t step = timeslice;
    while(step > 0) {
        switch(with->tag) {
            case APP:
                PUSH(with->t2);
                WITH(with->t1);
                NEXT(1);
            case I: 
                RESERVE(1);
                // I x = x
                WITH(ARG_1);
                POP(1);
                NEXT(1);
            case K:
                RESERVE(2);
                // K x y = x
                WITH(ARG_1);
                POP(2);
                NEXT(1);
            case S:
                RESERVE(3);
                // S x y z = (x z) (y z)
                PUSH(new_app(ARG_2,ARG_3));
                PUSH(ARG_3);
                WITH(ARG_1);
                POP(3);
                NEXT(2);
            case ADDI:
                RESERVE(2);
                if(ARG_1->tag != INT) { CALL(ARG_1); }
                if(ARG_2->tag != INT) { CALL(ARG_2); }
                WITH(new_int(ARG_1->int_v + ARG_2->int_v));
                POP(2);
                NEXT(2);
            case EXIT:
                task_return = NULL;
                goto save_content;
            case SYMB: {
                Dict_t* dict = dict_get(with->symb_v);
                if(dict != NULL) {
                    assert(dict->compiled != NULL);
                    LOG("dynamic linking %s ...\n",with->symb_v);
                    WITH(dict->compiled);
                    NEXT(3);
                } else {
                    PANIC("undefined symbol!\n");
                }
            }
            case INT:
            case REAL:
            case CHAR:
            case BOOL:
                // BASIC DATA
                if(sp[0] == &FRAME) {
                    // cover the hole
                    for(int i=0; ; i++) {
                        assert(sp[-i] != &ROOT);
                        if(sp[-i] == &HOLE) {
                            sp[-i] = with;
                        }
                    }
                    NEXT(2);
                } else {
                    printf("DATA1: ");
                    show_term(with);
                    printf("\nDATA2:");
                    show_term(x);
                    PANIC("\nCannot apply data to data!\n");
                }
            default:
                PANIC("Unknown tag while evaling term!\n");
        }
    }

    save_content:
    PUSH(with);
    task->stack_base = stack_base;
    task->stack_ceil = stack_ceil;
    task->sp = sp;
    return NULL;
}

/////////////////////////
//    MULTITASKING     //
/////////////////////////

#define NUM_THREADS 8
#define TASK_QUEUE_LEN 256
pthread_t thread_pool[NUM_THREADS];
Task_t* task_queue[TASK_QUEUE_LEN];
const Task_t TASK_QUEUE_END = task_queue + TASK_QUEUE_LEN;
task_t* head_task; // 第一个任务指针（如果有任务的话）
task_t* task_tail; // 最后一个任务指针的后面一格
// 当head_task == task_tail，池中没有任务

atomic_flag task_pool_lock = ATOMIC_FLAG_INIT;

#define task_pool_lock() \
    while(atomic_flag_test_and_set(&task_pool_lock)){}

#define task_pool_unlock() \
    atomic_flag_clear(&task_pool_lock)

void task_module_init() {
    head_task = task_queue;
    task_tail = task_queue;
    for(int i=0; i<NUM_THREADS; i++){
        //参数依次是：创建的线程id，线程参数，调用的函数，传入的函数参数
        int ret = pthread_create(&thread_pool[i], NULL, thread_loop, NULL);
        if (ret != 0) {
            printf("pthread_create error: %d\n", ret);
        }
    }
}

void task_module_exit() {
    for(int i=0; i<NUM_THREADS; i++){
        pthread_cancel(thread_pool[i]);
    }
}

void send_task(task_t task){
    task_pool_lock();
    if(task_tail < TASK_QUEUE_END) {
        *task_tail++ = task;
    } else {
        task_tail = task_queue;
        *task_tail++ = task;
    }
    task_pool_unlock();
}

void* thread_loop(){
    while(true) {
        task_pool_lock();
        if(head_task == task_tail) {
            // 进程池中没有任务则休眠1秒
            task_pool_unlock();
            sleep(1);
        } else {
            // fetch a task
            Task_t task = *head_task;
            if(head_task < TASK_QUEUE_END) {
                head_task ++;
            } else {
                head_task = task_queue; // 指针溢出，复位
            }
            task_pool_unlock();
            task = eval(task,256); // 执行任务
            if(task != NULL) {
                // send back the task


            }
        }
    }
}