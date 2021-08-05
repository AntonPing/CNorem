#include "Norem.h"

#define APP2(t1,t2) \
    new_app(t1,t2)

#define APP3(t1,t2,t3) \
    new_app(new_app(t1,t2),t3)

#define APP4(t1,t2,t3,t4) \
    new_app(new_app(new_app(t1,t2),t3),t4)

bool is_app(Term_t* term) {
    return term->tag == APP;
}
bool is_lamb(Term_t* term) {
    return term->tag == LAMB;
}
bool is_var(Term_t* term) {
    return term->tag == SYMB;
}

bool is_free_in(symb_t x, Term_t* term) {
    assert(term != NULL);
    if(is_var(term)) {
        return x == term->symb_v;
    } else if(is_lamb(term)) {
        if(term->x == x) {
            return false;
        } else {
            return is_free_in(x,term->t);
        }
    } else if(is_app(term)) {
        return is_free_in(x,term->t1) || is_free_in(x,term->t2);
    } else {
        return false;
    } 
}

Term_t* term_compile(Term_t* term) {
    assert(term != NULL);
    if(is_var(term)) {
        return term;
    } else if(is_lamb(term)) {
        symb_t x = term->x;
        Term_t* t = term->t;
        if(is_free_in(x, t)) {
            if(is_var(t)) {
                // we have term in form \x.y, and x is free in y
                // obviously x == y, it must be \x.x
                assert(x == t->symb_v);
                return &sing[I]; // T[\x.x] -> I
            } else if(is_lamb(t)) {
                // T[\x.\y.E] -> T[\x.T[\y.E]]
                return term_compile(new_lamb(x,term_compile(t)));
            } else if(is_app(t)) {
                // T[\x.(E1 E2)] -> (S T[\x.E1] T[\x.E2])
                return new_app(new_app(&sing[S],
                            term_compile(new_lamb(x,t->t1))),
                            term_compile(new_lamb(x,t->t2)));
            } else {
                // impossiable! x can't be free in constant.
                PANIC("something Wrong!\n");
            }
        } else {
            // T[\x.E] -> (K T[E]), if x is not free in E
            return new_app(&sing[K],t);
        }
    } else if(is_app(term)) { // for application
        // T[(E1 E2)] -> (T[E1] T[E2])
        return new_app(term_compile(term->t1),
                        term_compile(term->t2));
    } else {
        return term;
    }
}

Term_t* term_opt(Term_t* term) {
    // (S arg1 arg2)
    Term_t *arg1, *arg2, *p, *q, *r;
    if(term->tag == APP && term->t1->tag == APP
        && term->t1->t1->tag == S) {
        arg2 = term->t2;
        arg1 = term->t1->t2;
        // S (K p) I = p
        // S (K p) (K q) = K (p q)
        // S (K p) (B q r) = B* p q r
        // S (K p) q = B p q
        if(arg1->tag == APP && arg1->t1->tag == K) {
            p = arg1->t2;
            if(arg2->tag == I) {
            return p;
            } else if(arg2->tag == APP && arg2->t1->tag == K) {
                q = arg2->t2;
                return term_opt(APP2(&sing[K],APP2(p,q)));
            } else if(arg2->tag == APP && arg2->t1->tag == APP
                && arg2->t1->t1->tag == B) {
                r = arg2->t2;
                q = arg2->t1->t2;
                return term_opt(APP4(&sing[BS],p,q,r));
            } else {
                q = arg2;
                return term_opt(APP3(&sing[B],p,q));
            }
        // S (B p q) (K r) = C' p q r
        // S (B p q) r = S' p q r
        } else if(arg1->tag == APP && arg1->t1->tag == APP
                && arg1->t1->t1->tag == B) {
            q = arg1->t2;
            p = arg1->t1->t2;
            if(arg2->tag == APP && arg2->t1->tag == K) {
                r = arg2->t2;
                return term_opt(APP4(&sing[CP],p,q,r));
            } else {
                r = arg2;
                return term_opt(APP4(&sing[SP],p,q,r));
            }
        // S p (K q) = C p q
        } else {
            if(arg2->tag == APP && arg2->t1->tag == K) {
                q = arg2->t2;
                p = arg1;
                return term_opt(APP3(&sing[C],p,q));
            } else {
                // S p q ......
                q = arg2;
                p = arg1;
                return APP3(&sing[S],term_opt(p),term_opt(q));
            }
        }
    } else if(term->tag == APP) {
        Term_t* x1 = term_opt(term->t1);
        Term_t* x2 = term_opt(term->t2);
        if(x1 == term->t1 && x2 == term->t2) {
            return term;
        } else {
            return term_opt(APP2(x1,x2));
        }
    } else {
        return term;
    }
}


typedef struct Symb_List_t {
    symb_t this;
    struct Symb_List_t* next;
} Symb_List_t;

bool symb_list_lookup(symb_t symb, Symb_List_t* list) {
    Symb_List_t* look = list;
    while(true) {
        if(look == NULL) {
            return false;
        } else if(look->this == symb) {
            return true;
        } else {
            look = look->next;
        }
    }
}

Term_t* term_link_helper(Term_t* term, Symb_List_t* list) {
    PANIC("static linking disabled");
    if(term == NULL) {
        PANIC("NULL!\n");
    } else if(is_var(term)) {
        if(symb_list_lookup(term->symb_v,list)) {
            return term;
        } else {
            Dict_t* dict = dict_get(term->symb_v);
            if(dict == NULL) {
                LOG("can't find definition %s\n",term->symb_v);
                return NULL;
            } else {
                if(dict->linked == NULL) {
                    dict->linked = term_link(dict->raw);
                }
                if(dict->compiled == NULL) {
                    dict->compiled = term_compile(dict->linked);
                }
                return dict->compiled;
            }
        }
    } else if(is_lamb(term)) {
        Symb_List_t new_list = { term->x, list };
        return new_lamb(term->x, term_link_helper(term->t, &new_list));
    } else if(is_app(term)) {
        return new_app(term_link_helper(term->t1, list),
                       term_link_helper(term->t2, list));
    } else {
        return term;
    }
}

Term_t* term_link(Term_t* term) {
    PANIC("static linking disabled");
    return term_link_helper(term, NULL);
}


