#include "Norem.h"
#include <editline/readline.h>

Dict_t* root = NULL;

Dict_t* dict_get(symb_t key) {
    Dict_t* ptr = root;
    while(ptr != NULL) {
        if(ptr->name == key) {
            return ptr;
        } else {
            ptr = ptr->next;
        }
    }
    return NULL;
}

Dict_t* dict_new(symb_t key) {
    LOG("new key of dictionary: %s\n", key);
    assert(dict_get(key) == NULL);
    Dict_t* res = malloc(sizeof(Dict_t));
    res->name = key;
    res->raw = NULL;
    res->linked = NULL;
    res->compiled = NULL;
    res->text = NULL;
    res->next = root;
    root = res;
    return root;
}

void dict_define(symb_t key, string_t text, Term_t* value) {
    Dict_t* dict = dict_get(key);
    if(dict == NULL) {
        dict = dict_new(key);
        dict->text = text;
        dict->raw = value;
        dict->compiled = term_opt(term_compile(value));
        dict->linked = NULL;
        puts("Ok.");
    } else {
        puts("Defination already exist!");
    }
}

void dict_update(symb_t key, string_t text, Term_t* value) {
    Dict_t* dict = dict_get(key);
    if(dict != NULL) {
        dict->text = text;
        dict->raw = value;
        dict->compiled = term_opt(term_compile(value));
        dict->linked = NULL;
        puts("Ok.");
    } else {
        puts("Defination doesn't exist!");
    }
}

void command(string_t input);

// definition: from NoremParser.c
extern bool definition(char_t* str, symb_t* key, Term_t** value);

void command_define(string_t text, bool update) {
    symb_t key;
    Term_t* value;
    if(definition(text, &key, &value)) {
        if(update) {
            dict_update(key, text, value);
        } else {
            dict_define(key, text, value);
        }
    } else {
        puts("Command Parsing Error!");
    }
}

void command_relink() {
    PANIC("static linking disabled!\n");
    Dict_t* ptr = root;
    while(ptr != NULL) {
        ptr->linked = NULL;
        ptr = ptr->next;
    }
}

// is_space: from NoremParser.c
extern bool is_space(char_t c);

void command_load(string_t path) {
    FILE *fp = NULL;
    fp = fopen(path, "r");
    if(fp == NULL) {
        puts("file can't open!");
        return;
    } else {
        puts("reading from file...");
    }

    char_t buff[2048];
    size_t idx = 0;
    bool newline = false;

    while(idx <= 2047) {
        buff[idx] = (char_t)fgetc(fp);
        if(buff[idx] == ':'  && newline == true) {
            buff[idx] = '\0';
            command(buff);
            buff[0] = ':';
            newline = false;
            idx = 1;
        } else if(buff[idx] == EOF) {
            buff[idx] = '\0';
            command(buff);
            puts("file ends.");
            return;
        } else if(is_space(buff[idx])) {
            newline = true;
            idx ++;
        } else {
            newline = false;
            idx ++;
        }
    }
    puts("buffer overflow!");
}

void command(string_t input) {
    if(strncmp(input,":quit",5) == 0) {
        puts("Goodbye!");
        task_module_exit();
        exit(0);
    } else if(strncmp(input,":dict -c",8) == 0) {
        show_dict_compiled(root);
    } else if(strncmp(input,":dict",5) == 0) {
        show_dict_raw(root);
    } else if(strncmp(input,":load ",6) == 0) {
        command_load(&input[6]);
    } else if(strncmp(input,":define ",8) == 0) {
        command_define(&input[8], false);
    } else if(strncmp(input,":update ",8) == 0) {
        command_define(&input[8], true);
    } else if(strncmp(input,":relink",7) == 0) {
        command_relink();
    } else {
        printf("Error: Unknown command! {\n %s\n}\n", input);
    }
}

// heap_init: from NoremHeap.c
extern void heap_init();
// eval: from NoremEval.c
extern bool eval(Task_t* task, int_t timeslice);
// term_parse: from NoremParser.c
extern bool term_parse(char_t* str, Term_t** ret);

void repl() {
    // Print Version and Exit Information
    puts("Norem Repl Version 0.1");
    puts("Type :quit to Exit\n");

    heap_init();
    task_module_init();

    // THIS IS FOR TESTING SAKE
    command(":load test.nrm");
    // YOU MAY COMMENT IT

    while(true) {

        // Output our prompt and get input
        char* input = readline("> ");
        add_history(input);

        if(input[0] == ':') {
            command(input);
        } else {
            Term_t* term;
            Task_t* task;
            if(term_parse(input,&term)) {
                //command_relink();
                term = term_compile(term);
                term = term_opt(term);
                //term = term_link(term);
                task = new_task(term);

                //eval_start:
                if(eval(task, 1024)) {
                    send_task(task);
                    /*
                    char yes_or_no;
                    // One-Thread-Mode
                    // goto eval_start;
                    //
                    puts("This task seems doesn't terminate, do you want to "
                        "dumped it into the thread queue? (y/n)");
                    while(true) {
                        scanf("%c", &yes_or_no);
                        if(yes_or_no == 'y') {
                            send_task(task);
                            puts("Ok.");
                            break;
                        } else if(yes_or_no == 'n') {
                            goto eval_start;
                        } else {
                            puts("Please type 'y' or 'n' for yes or no!");
                            continue;
                        }
                    }
                    */
                } else {
                    show_term(task->ret);
                    printf("\n");
                    free(task);
                }
                //show_term(term);
                //printf("\n");
            } else {
                puts("Parser Error!"); 
            }
        }
        // Free retrieved input
        free(input);
    }
    task_module_exit();
    
}


int main() {
    repl();
    return 0;
}
