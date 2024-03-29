/*
This Scheme Interprter borrows Code from  Peter Michaux Scheme Interpreter:
https://github.com/petermichaux/bootstrap-scheme/tree/master
-->floats still remain to be added, Garbage Collector is not bug free yet.
*/

/**Features*/ //
//Automatic Garbage Collector --> no totally bug free yet
/*
> #t
#t
> 1
1
> #\c
#\c
> "aaaaa"
"aaaaa"
> (quote (0 . 1))
(0 . 1)
> (define a 3)
ok
> a
3
> (set! a 1)
ok
> a
1
> (if #t 0 1)
0
> (+ 1 2 3)
6
> ((lambda (x y) (+ x y))
   1 2)
3
> (define (mul x y) (* x y))
ok
> (mul 2 3)
6
> (begin 1 2)
2
> (cond (#f          1)
        ((eq? #t #t) 2)
        (else        3))
2
> (let ((x (+ 1 1))
        (y (- 5 2)))
    (+ x y))
5
*/
#include <stdlib.h> // standard
#include <stdio.h>  //input output
#include <string.h> //string
#include <ctype.h>  //testing and mapping characterss

/* DEBUG VARS*/
int number_of_marked_objects = 0;

/**************************** MODEL ******************************/

//define enum with name "object_type" with only option FIXNUM as choice
typedef enum
{
    THE_EMPTY_LIST,
    BOOLEAN,
    SYMBOL, //linked to value
    FIXNUM,
    FLOATNUM, //not implemented yet
    CHARACTER,
    STRING,         //linked to value
    PAIR,           //linked to car and cdr
    PRIMITIVE_PROC, //linked to function
    COMPOUND_PROC,  //linked to param, body, env
} object_type;

typedef struct schemeobject
{
    object_type type;
    //A union is a type consisting of a sequence of members whose storage overlaps
    //union is named data
    int mark;
    struct schemeobject *next;
    struct schemeobject *next_root;

    struct
    {
        //A struct is a type consisting of a sequence of members whose storage is allocated in an ordered sequence
        struct
        {
            char value;
        } boolean;
        struct
        {
            char *value;
        } symbol;
        struct
        {
            long value;
        } fixnum;
        struct
        {
            long value;
        } floatnum;
        struct
        {
            char value;
        } character;
        struct
        {
            char *value;
        } string;
        struct
        {
            struct schemeobject *car;
            struct schemeobject *cdr;
        } pair;
        struct
        {
            struct schemeobject *(*fn)(struct schemeobject *arguments);
        } primitive_proc;
        struct
        {
            struct schemeobject *parameters;
            struct schemeobject *body;
            struct schemeobject *env;
        } compound_proc;
        struct
        {
            FILE *stream;
        } input_port;
        struct
        {
            FILE *stream;
        } output_port;
    } data;
} schemeobject;

//some GC FUnktions
//keeping track of the roots for the mark-and-sweep algorithm.
//The root objects are the ones where the mark phase starts.
//These are objects that may not have any other objects pointing to them but should still not be garbage collected

//Let's define a linked list object:

schemeobject *free_list;
//define active List
schemeobject *active_list;
///define root_list where mark steps starts from
schemeobject *root_list;

const int number_of_objects = 1040; //1023 to test gc

/*function declaration*/
char is_the_empty_list(schemeobject *obj);
schemeobject *cdr(schemeobject *pair);

//print list
/* Counts no. of nodes in linked list */
int getCount(schemeobject *head)
{
    int count = 0;                // Initialize count
    schemeobject *current = head; // Initialize current
    //printf("List count start is at address: %p\n", (void *)current);
    //printf("List count start->next is at address: %p\n", (void *)current->next);

    while (current != NULL)
    {
        count++;
        current = current->next;
        /* if (count==1498){
                printf("List count last is at address: %p\n", (void *)current);

        }*/
    }
    return count;
}
int getRootCount()
{
    int count = 0;                     // Initialize count
    schemeobject *current = root_list; // Initialize current
    //printf("List count start is at address: %p\n", (void *)current);
    //printf("List count start->next is at address: %p\n", (void *)current->next);

    while (current != NULL)
    {
        count++;
        current = current->next_root;
    }
    return count;
}
int getCountCdr(schemeobject *head)
{
    int countcdr = 0;
    while (!is_the_empty_list(head))
    {
        countcdr++;
        head = cdr(head);
    };
    return countcdr;
}
void print_next_list(schemeobject *head)
{
    schemeobject *current = head;
    int i = 0;
    while (current != NULL)
    {
        printf("List item %d adress: %p\n", i, (void *)current);
        current = current->next;
        i++;
    }
}
//print number of free items
void print_objects_status()
{
    int number_of_free = getCount(free_list);
    int number_of_active = getCount(active_list);
    float percentage_used = (float)number_of_active / number_of_objects * 100.0;

    printf("%s", "Number of free Objects available: ");
    printf("%d\n", number_of_free);
    printf("%s", "Number of used Objects: ");
    printf("%d\n", number_of_active);
    printf("Percentage used = %.2f%%\n", percentage_used);
}
//add item to end of list
void push_newschemeobject_to_linked_list(schemeobject *head, int mark)
{
    schemeobject *current = head;
    while (current->next != NULL)
    {
        current = current->next;
    }

    /* now we can add a new variable */
    current->next = malloc(sizeof(schemeobject));
    current->next->mark = mark;
    current->next->next = NULL;
}

void push_pointer_to_existing_schemeobject_to_root_list(schemeobject *rootobject)
{
    schemeobject *current = root_list;
    if (root_list == NULL)
    {
        root_list = rootobject;
    }

    else
    { //goto end of root list
        while (current->next_root != NULL)
        {
            current = current->next_root;
        }
        /* now we can add a new variable */
        current->next_root = rootobject;
        current->next_root->next_root = NULL;
    }
}

schemeobject *the_empty_list;
//*****************boolean//
schemeobject * false;
schemeobject * true;
//symbol
schemeobject *old_symbol_table;
schemeobject *symbol_table;

schemeobject *cons(schemeobject *car, schemeobject *cdr);
schemeobject *car(schemeobject *pair);
schemeobject *cdr(schemeobject *pair);
//quote
schemeobject *quote_symbol;
//environment
schemeobject *define_symbol;
schemeobject *set_symbol;
schemeobject *ok_symbol;
schemeobject *the_empty_environment;
schemeobject *the_global_environment;
//if
schemeobject *if_symbol;
//lambda
schemeobject *lambda_symbol;
//begin
schemeobject *begin_symbol;
//cond
schemeobject *cond_symbol;
schemeobject *else_symbol;
//let
schemeobject *let_symbol;
//and or
schemeobject *and_symbol;
schemeobject *or_symbol;

char is_the_empty_list(schemeobject *obj)
{
    return obj == the_empty_list;
}
//some gc functions
int alreadymarked;
void mark_binary_tree(schemeobject *tree_root)
{
    //fprintf(stderr, "marking\n");

    if (tree_root->mark == 1)
    {
        alreadymarked++;
    }
    tree_root->mark = 1;

    number_of_marked_objects++;
    //check oberer "ast" des trees:
    if (car(tree_root) != NULL)
    {
        //fprintf(stderr, "mark car\n");
        mark_binary_tree(car(tree_root));
    }
    //check unterer "ast" des trees:
    if (cdr(tree_root) != NULL)
    {
        //fprintf(stderr, "mark cdr\n");
        mark_binary_tree(cdr(tree_root));
    }
}
void setActiveListMarksZero()
{
    schemeobject *currentactive = active_list;
    while (currentactive != NULL)
    {
        currentactive->mark = 0;
        currentactive = currentactive->next;
    }
}
//gc function,
void gc()
{
    //markphase: traverse explicit through all reachable objects, starting from rootobjects.
    //As second step go through stack objects
    schemeobject *current = root_list;
    number_of_marked_objects = 0;

    //assuming there are 5 root objects including global_env and symbol_table
    while (current != NULL)
    {
        schemeobject *currentobject = current;

        //for each root_item
        //check if item has pointers to other objects and mark each reachable object (mark=1)
        alreadymarked = 0;
        mark_binary_tree(currentobject);
        //printf("number_of_already markred %d\n", alreadymarked);
        current = current->next_root;
        //printf("number_of_marked_objects %d\n", number_of_marked_objects);
    }
    printf("number_of_marked_objects %d\n", number_of_marked_objects);

    //stack objects
    //-->to be implemented

    //sweepphase:
    /*In the sweep phase, the garbage collector starts at the top of the active_list
    and examines each object in that list. Objects that have mark set to 0 are moved to the free_list. 
    (Symbols and strings also need to have their data.symbol.value field, for example, freed.) 
    Objects in the active_list that have mark set to 1 have their mark set back to 0 in preparation for the next mark phase.
    */
    //return if active list is empty
    if (active_list == NULL)
    {
        return;
    }

    int number_of_moved_items = 0;
    //take take of first item: look if first item in active list is not marked
    while (active_list->mark == 0)
    {
        //back from active to free List:
        //save (currentactive) adress:
        schemeobject *object_to_move = active_list;
        //drop object_to_move from active list:
        active_list = active_list->next;
        //drop next reference? why here already necessary?
        object_to_move->next = NULL;
        //make sure mark is zero
        object_to_move->mark = 0;
        //move temp to free list:
        //check if free list is NULL:
        if (free_list == NULL)
        {
            //init free List:
            free_list = object_to_move;
        }
        else
        {
            //add to free List:

            schemeobject *currentfree = free_list;

            while (currentfree->next != NULL)
            {
                currentfree = currentfree->next;
            }
            //add active item to last item in free list
            currentfree->next = object_to_move;
            //print_objects_status();

            currentfree->next->next = NULL;
        }
        number_of_moved_items++;
        //fprintf(stderr, "moved first item of active\n");
        //print_objects_status();
    }

    //now take care of other list items, we are sure active_list->mark is 0
    //traverse through active_list
    schemeobject *currentactive = active_list;
    while (currentactive != NULL && currentactive->next != NULL)
    {
        //for each active List item:
        //check if mark is still 0, after mark phase:
        if (currentactive->next->mark == 0)
        {
            //back from active to free List:
            //save (currentactive->next) adress:
            schemeobject *object_to_move = currentactive->next;
            //drop object_to_move from active list:
            currentactive->next = currentactive->next->next;
            //drop next reference? why here already necessary?
            object_to_move->next = NULL;
            //make sure mrk is zero
            object_to_move->mark = 0;
            //move temp to free list:
            //check if free list is NULL:
            if (free_list == NULL)
            {
                //init free List:
                free_list = object_to_move;
            }
            else
            { //add to free List:

                schemeobject *currentfree = free_list;

                while (currentfree->next != NULL)
                {
                    currentfree = currentfree->next;
                }
                //add active item to last item in free list
                currentfree->next = object_to_move;
                //print_objects_status();

                currentfree->next->next = NULL;
            }
            number_of_moved_items++;
            //fprintf(stderr, "moved one middle item of active\n");
            //print_objects_status();
        }

        currentactive = currentactive->next;
    }
    setActiveListMarksZero();
    printf("number_of_moved_items %d\n", number_of_moved_items);
}

//alloc object , later GC
schemeobject *alloc_object(int use_gc)
{
    //look if free_list has at least one available object
    if (free_list != NULL)
    {
        //found free schemeobject!
        //save adress of free object
        schemeobject *tempfree = free_list;

        //printf("variable free_list is at address: %p\n", (void *)free_list);
        //printf("variable tempfree is at address: %p\n", (void *)tempfree);
        //printf("variable free_list->next is at address: %p\n", (void *)free_list->next);

        //drop first object from free list:
        free_list = free_list->next;
        //drop next reference? why here already necessary?
        tempfree->next = NULL;

        //printf("variable free_list AFTER is at address: %p\n", (void *)free_list);
        //printf("variable tempfree AFTER is at address: %p\n", (void *)tempfree);
        //printf("variable free_list->next AFTER is at address: %p\n", (void *)free_list->next);

        //move temp to active list:
        //check if active list is NULL:
        if (active_list == NULL)
        {
            //init active List:
            active_list = tempfree;
        }
        else
        { //add to active List:

            schemeobject *currentactive = active_list;

            while (currentactive->next != NULL)
            {
                currentactive = currentactive->next;
            }
            //add free item to last item in active list
            currentactive->next = tempfree;
            //print_objects_status();

            currentactive->next->next = NULL;
        }

        //return schemeobject to use
        //printf("%d\n", current->mark);

        //init needed references to Null to avoid 0xbaadfood-error(using not allocated memory on the heap)
        tempfree->next_root = NULL;
        tempfree->data.pair.car = NULL;
        tempfree->data.pair.cdr = NULL;

        return tempfree;
    }
    //free list has no object to offer, start GC!
    if (use_gc == 1)
    {
        //printf("%s%d%s", "symbol_table_length: ", getCountCdr(symbol_table), "\n");

        fprintf(stderr, "starting gc\n");

        gc();
        fprintf(stderr, "GC DONE\n");
        //printf("%s%d%s", "symbol_table_length: ", getCountCdr(symbol_table), "\n");

        return alloc_object(0);
    }

    fprintf(stderr, "out of memory while allocationg\n");
    exit(1);
}

char is_boolean(schemeobject *obj)
{
    return obj->type == BOOLEAN;
}

char is_false(schemeobject *obj)
{
    return obj == false;
}

char is_true(schemeobject *obj)
{
    return !is_false(obj);
}

void updateRootList(schemeobject *old_symbol_table)
{
    schemeobject *currentRoot = root_list;
    //case rootList is null
    if (root_list == NULL)
    {
        root_list = symbol_table;
    }
    //case root list has length 1
    else if (root_list->next_root == NULL)
    {
        if (root_list = old_symbol_table)
        {
            root_list = symbol_table;
        }
        else
        {
            root_list->next_root = symbol_table;
        }
    }
    else
    { //case old_symbol_table is first item
        if (root_list == old_symbol_table)
        {
            schemeobject *tempEnd = root_list->next_root;
            root_list = symbol_table;
            root_list->next_root = tempEnd;
        }
        else
        {
            //scan for old_symbol_table
            while (currentRoot->next_root != old_symbol_table)
            {
                currentRoot = currentRoot->next_root;
            }
            //we assume now: currentRoot->next_root = old_symbol_table
            schemeobject *temp_end = currentRoot->next_root->next_root;
            currentRoot->next_root = symbol_table;
            currentRoot->next_root->next_root = temp_end;
        }
    }
}
//symbol
int number_of_made_symbols = 0;

schemeobject *make_symbol(char *value)
{
    //printf("%s%d%s", "symbol_table_length: ", getCountCdr(symbol_table), "\n");

    schemeobject *obj;
    schemeobject *element;
    /* search for the symbol in the symbol table */
    element = symbol_table;
    while (!is_the_empty_list(element))
    {
        //printf("%s", "!is_the_empty_list: ");

        if (strcmp(car(element)->data.symbol.value, value) == 0)
        {
            //printf("%s", "end of make symbol- old symbol: \n");

            return car(element);
        }
        element = cdr(element);
    };

    /* create the symbol and add it to the symbol table */
    obj = alloc_object(1);
    obj->type = SYMBOL;
    obj->data.symbol.value = malloc(strlen(value) + 1);
    if (obj->data.symbol.value == NULL)
    {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    strcpy(/*dest*/ obj->data.symbol.value, /*source*/ value);
    old_symbol_table = symbol_table;
    symbol_table = cons(obj, symbol_table);
    updateRootList(old_symbol_table);
    //printf("%s", "end of make symbol- new symbol: ");

    number_of_made_symbols++;
    //printf("%s", "number_of_made_symbols: ");
    //printf("%d\n", number_of_made_symbols);
    return obj;
}

char is_symbol(schemeobject *obj)
{
    return obj->type == SYMBOL;
}
///**fixnum
schemeobject *make_fixnum(long value)
{
    schemeobject *obj;

    obj = alloc_object(1);
    obj->type = FIXNUM;
    obj->data.fixnum.value = value;
    return obj;
}

char is_fixnum(schemeobject *obj)
{
    return obj->type == FIXNUM;
}

schemeobject *make_char(char value)
{
    schemeobject *obj;

    obj = alloc_object(1);
    obj->type = CHARACTER;
    obj->data.character.value = value;
    return obj;
}

schemeobject *make_str(char *value)
{
    schemeobject *obj;

    obj = alloc_object(1);
    obj->type = STRING;
    obj->data.string.value = malloc(strlen(value) + 1);
    if (obj->data.string.value == NULL)
    {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    strcpy(obj->data.string.value, value);
    return obj;
}

char is_str(schemeobject *obj)
{
    return obj->type == STRING;
}

char is_character(schemeobject *obj)
{
    return obj->type == CHARACTER;
}

//PAIR = make_pair
schemeobject *cons(schemeobject *car, schemeobject *cdr)
{
    schemeobject *obj;

    obj = alloc_object(1);
    obj->type = PAIR;
    obj->data.pair.car = car;
    obj->data.pair.cdr = cdr;
    return obj;
}

char is_pair(schemeobject *obj)
{
    return obj->type == PAIR;
}

schemeobject *car(schemeobject *pair)
{
    return pair->data.pair.car;
}

void set_car(schemeobject *obj, schemeobject *value)
{
    obj->data.pair.car = value;
}

schemeobject *cdr(schemeobject *pair)
{
    return pair->data.pair.cdr;
}

void set_cdr(schemeobject *obj, schemeobject *value)
{
    obj->data.pair.cdr = value;
}

#define caar(obj) car(car(obj))
#define cadr(obj) car(cdr(obj))
#define cdar(obj) cdr(car(obj))
#define cddr(obj) cdr(cdr(obj))
#define caaar(obj) car(car(car(obj)))
#define caadr(obj) car(car(cdr(obj)))
#define cadar(obj) car(cdr(car(obj)))
#define caddr(obj) car(cdr(cdr(obj)))
#define cdaar(obj) cdr(car(car(obj)))
#define cdadr(obj) cdr(car(cdr(obj)))
#define cddar(obj) cdr(cdr(car(obj)))
#define cdddr(obj) cdr(cdr(cdr(obj)))
#define caaaar(obj) car(car(car(car(obj))))
#define caaadr(obj) car(car(car(cdr(obj))))
#define caadar(obj) car(car(cdr(car(obj))))
#define caaddr(obj) car(car(cdr(cdr(obj))))
#define cadaar(obj) car(cdr(car(car(obj))))
#define cadadr(obj) car(cdr(car(cdr(obj))))
#define caddar(obj) car(cdr(cdr(car(obj))))
#define cadddr(obj) car(cdr(cdr(cdr(obj))))
#define cdaaar(obj) cdr(car(car(car(obj))))
#define cdaadr(obj) cdr(car(car(cdr(obj))))
#define cdadar(obj) cdr(car(cdr(car(obj))))
#define cdaddr(obj) cdr(car(cdr(cdr(obj))))
#define cddaar(obj) cdr(cdr(car(car(obj))))
#define cddadr(obj) cdr(cdr(car(cdr(obj))))
#define cdddar(obj) cdr(cdr(cdr(car(obj))))
#define cddddr(obj) cdr(cdr(cdr(cdr(obj))))

//simple procedures

//wants pointer to a funtion which returns a pointer to a scheme object and takes a schemeobject as argument
schemeobject *make_primitive_proc(schemeobject *(*fn)(schemeobject *arguments))
{
    schemeobject *obj;

    obj = alloc_object(1);
    obj->type = PRIMITIVE_PROC;
    obj->data.primitive_proc.fn = fn;
    return obj;
}

char is_primitive_proc(schemeobject *obj)
{
    return obj->type == PRIMITIVE_PROC;
}

schemeobject *is_null_proc(schemeobject *arguments)
{
    return is_the_empty_list(car(arguments)) ? true : false;
}

schemeobject *is_boolean_proc(schemeobject *arguments)
{
    return is_boolean(car(arguments)) ? true : false;
}

schemeobject *is_symbol_proc(schemeobject *arguments)
{
    return is_symbol(car(arguments)) ? true : false;
}

schemeobject *is_integer_proc(schemeobject *arguments)
{
    return is_fixnum(car(arguments)) ? true : false;
}

schemeobject *is_char_proc(schemeobject *arguments)
{
    return is_character(car(arguments)) ? true : false;
}

schemeobject *is_string_proc(schemeobject *arguments)
{
    return is_str(car(arguments)) ? true : false;
}

schemeobject *is_pair_proc(schemeobject *arguments)
{
    return is_pair(car(arguments)) ? true : false;
}

char is_compound_proc(schemeobject *obj);

schemeobject *is_procedure_proc(schemeobject *arguments)
{
    schemeobject *obj;

    obj = car(arguments);
    return (is_primitive_proc(obj) ||
            is_compound_proc(obj))
               ? true
               : false;
}

schemeobject *char_to_integer_proc(schemeobject *arguments)
{
    return make_fixnum((car(arguments))->data.character.value);
}

schemeobject *integer_to_char_proc(schemeobject *arguments)
{
    return make_char((car(arguments))->data.fixnum.value);
}

schemeobject *number_to_string_proc(schemeobject *arguments)
{
    char buffer[100];

    sprintf(buffer, "%ld", (car(arguments))->data.fixnum.value);
    return make_str(buffer);
}

schemeobject *string_to_number_proc(schemeobject *arguments)
{
    return make_fixnum(atoi((car(arguments))->data.string.value));
}

schemeobject *symbol_to_string_proc(schemeobject *arguments)
{
    return make_str((car(arguments))->data.symbol.value);
}

schemeobject *string_to_symbol_proc(schemeobject *arguments)
{
    return make_symbol((car(arguments))->data.string.value);
}

schemeobject *add_proc(schemeobject *arguments)
{
    long result = 0;

    while (!is_the_empty_list(arguments))
    {
        result += (car(arguments))->data.fixnum.value;
        arguments = cdr(arguments);
    }
    return make_fixnum(result);
}

schemeobject *sub_proc(schemeobject *arguments)
{
    long result;

    result = (car(arguments))->data.fixnum.value;
    while (!is_the_empty_list(arguments = cdr(arguments)))
    {
        result -= (car(arguments))->data.fixnum.value;
    }
    return make_fixnum(result);
}

schemeobject *mul_proc(schemeobject *arguments)
{
    long result = 1;

    while (!is_the_empty_list(arguments))
    {
        result *= (car(arguments))->data.fixnum.value;
        arguments = cdr(arguments);
    }
    return make_fixnum(result);
}

schemeobject *quotient_proc(schemeobject *arguments)
{
    return make_fixnum(
        ((car(arguments))->data.fixnum.value) /
        ((cadr(arguments))->data.fixnum.value));
}

schemeobject *remainder_proc(schemeobject *arguments)
{
    return make_fixnum(
        ((car(arguments))->data.fixnum.value) %
        ((cadr(arguments))->data.fixnum.value));
}

schemeobject *is_number_equal_proc(schemeobject *arguments)
{
    long value;

    value = (car(arguments))->data.fixnum.value;
    while (!is_the_empty_list(arguments = cdr(arguments)))
    {
        if (value != ((car(arguments))->data.fixnum.value))
        {
            return false;
        }
    }
    return true;
}

schemeobject *is_less_than_proc(schemeobject *arguments)
{
    long previous;
    long next;

    previous = (car(arguments))->data.fixnum.value;
    while (!is_the_empty_list(arguments = cdr(arguments)))
    {
        next = (car(arguments))->data.fixnum.value;
        if (previous < next)
        {
            previous = next;
        }
        else
        {
            return false;
        }
    }
    return true;
}

schemeobject *is_greater_than_proc(schemeobject *arguments)
{
    long previous;
    long next;

    previous = (car(arguments))->data.fixnum.value;
    while (!is_the_empty_list(arguments = cdr(arguments)))
    {
        next = (car(arguments))->data.fixnum.value;
        if (previous > next)
        {
            previous = next;
        }
        else
        {
            return false;
        }
    }
    return true;
}

schemeobject *cons_proc(schemeobject *arguments)
{
    return cons(car(arguments), cadr(arguments));
}

schemeobject *car_proc(schemeobject *arguments)
{
    return caar(arguments);
}

schemeobject *cdr_proc(schemeobject *arguments)
{
    return cdar(arguments);
}

schemeobject *set_car_proc(schemeobject *arguments)
{
    set_car(car(arguments), cadr(arguments));
    return ok_symbol;
}

schemeobject *set_cdr_proc(schemeobject *arguments)
{
    set_cdr(car(arguments), cadr(arguments));
    return ok_symbol;
}

schemeobject *list_proc(schemeobject *arguments)
{
    return arguments;
}

schemeobject *is_eq_proc(schemeobject *arguments)
{
    schemeobject *obj1;
    schemeobject *obj2;

    obj1 = car(arguments);
    obj2 = cadr(arguments);

    if (obj1->type != obj2->type)
    {
        return false;
    }
    switch (obj1->type)
    {
    case FIXNUM:
        return (obj1->data.fixnum.value ==
                obj2->data.fixnum.value)
                   ? true
                   : false;
        break;
    case CHARACTER:
        return (obj1->data.character.value ==
                obj2->data.character.value)
                   ? true
                   : false;
        break;
    case STRING:
        return (strcmp(obj1->data.string.value,
                       obj2->data.string.value) == 0)
                   ? true
                   : false;
        break;
    default:
        return (obj1 == obj2) ? true : false;
    }
}
//apply
schemeobject *apply_proc(schemeobject *arguments)
{
    fprintf(stderr, "illegal state: The body of the apply "
                    "primitive procedure should not execute.\n");
    exit(1);
}
//eval
schemeobject *interaction_environment_proc(schemeobject *arguments)
{
    return the_global_environment;
}

schemeobject *setup_environment(void);

schemeobject *null_environment_proc(schemeobject *arguments)
{
    return setup_environment();
}

schemeobject *make_environment(void);

schemeobject *environment_proc(schemeobject *arguments)
{
    return make_environment();
}

schemeobject *eval_proc(schemeobject *arguments)
{
    fprintf(stderr, "illegal state: The body of the eval "
                    "primitive procedure should not execute.\n");
    exit(1);
}

//i/o
schemeobject *read(FILE *in);
schemeobject *eval(schemeobject *exp, schemeobject *env);

//to load scm library
schemeobject *load_proc_from_str(char *filename)
{
    FILE *in;
    schemeobject *exp;
    schemeobject *result;

    in = fopen(filename, "r");
    if (in == NULL)
    {
        fprintf(stderr, "could not load file \"%s\"", filename);
        exit(1);
    }
    while ((exp = read(in)) != NULL)
    {
        result = eval(exp, the_global_environment);
    }
    fclose(in);
    return result;
}

int peek(FILE *in);

void write(FILE *out, schemeobject *obj);

schemeobject *make_compound_proc(schemeobject *parameters, schemeobject *body,
                                 schemeobject *env)
{
    schemeobject *obj;

    obj = alloc_object(1);
    obj->type = COMPOUND_PROC;
    obj->data.compound_proc.parameters = parameters;
    obj->data.compound_proc.body = body;
    obj->data.compound_proc.env = env;
    return obj;
}

char is_compound_proc(schemeobject *obj)
{
    return obj->type == COMPOUND_PROC;
}

//environment
schemeobject *enclosing_environment(schemeobject *env)
{
    return cdr(env);
}

schemeobject *first_frame(schemeobject *env)
{
    return car(env);
}

schemeobject *make_frame(schemeobject *variables, schemeobject *values)
{
    return cons(variables, values);
}

schemeobject *frame_variables(schemeobject *frame)
{
    return car(frame);
}

schemeobject *frame_values(schemeobject *frame)
{
    return cdr(frame);
}

void add_binding_to_frame(schemeobject *var, schemeobject *val,
                          schemeobject *frame)
{

    set_car(/*object*/ frame, /*value added to object*/ cons(var, car(frame)));
    set_cdr(/*object*/ frame, cons(val, cdr(frame)));
}

schemeobject *init_environment(schemeobject *vars, schemeobject *vals,
                                 schemeobject *base_env)
{
    //fprintf(stderr, "init_environment\n");
    return cons(make_frame(vars, vals), base_env);
}

schemeobject *lookup_variable_value(schemeobject *var, schemeobject *env)
{
    schemeobject *frame;
    schemeobject *vars;
    schemeobject *vals;
    while (!is_the_empty_list(env))
    {
        frame = first_frame(env);
        vars = frame_variables(frame);
        vals = frame_values(frame);
        while (!is_the_empty_list(vars))
        {
            if (var == car(vars))
            {
                return car(vals);
            }
            vars = cdr(vars);
            vals = cdr(vals);
        }
        env = enclosing_environment(env);
    }
    fprintf(stderr, "unbound variable\n");
    exit(1);
}

void set_variable_value(schemeobject *var, schemeobject *val, schemeobject *env)
{
    schemeobject *frame;
    schemeobject *vars;
    schemeobject *vals;

    while (!is_the_empty_list(env))
    {
        frame = first_frame(env);
        vars = frame_variables(frame);
        vals = frame_values(frame);
        while (!is_the_empty_list(vars))
        {
            if (var == car(vars))
            {
                set_car(vals, val);
                return;
            }
            vars = cdr(vars);
            vals = cdr(vals);
        }
        env = enclosing_environment(env);
    }
    fprintf(stderr, "unbound variable\n");
    exit(1);
}

void define_variable(/*symbol*/ schemeobject *var, /*proc*/ schemeobject *val, /*env*/ schemeobject *env)
{
    //char z[100] = "def_var.";
    //printf("%s", z); // %s is format specifier
    //printf("The value of env Next  at start of define_variable: %p\n", (void *)env->next);
    //printf("The value of env       at start of define_variable: %p\n", (void *)env);

    schemeobject *frame;
    schemeobject *vars;
    schemeobject *vals;

    frame = first_frame(env);
    vars = frame_variables(frame);
    vals = frame_values(frame);

    //scan env for variable
    while (!is_the_empty_list(vars))
    {
        if (var == car(vars))
        {
            set_car(vals, val);
            //variable already defined
            return;
        }
        vars = cdr(vars);
        vals = cdr(vals);
    }
    add_binding_to_frame(var, val, frame);
}

schemeobject *setup_environment(void)
{
    schemeobject *initial_env;

    initial_env = init_environment(
        the_empty_list,
        the_empty_list,
        the_empty_environment);

    return initial_env;
}
//eval
void populate_environment(schemeobject *env)
{

#define add_procedure(scheme_name, c_name)       \
    define_variable(make_symbol(scheme_name),    \
                    make_primitive_proc(c_name), \
                    env);
    //printf("The value of populate_environment Next before is_null_proc: %p\n", (void *)env->next);

    add_procedure("null?", is_null_proc);
    //printf("The value of populate_environment Next after is_null_proc: %p\n", (void *)env->next);
    //-->symbols are added to global env as binary tree
    add_procedure("boolean?", is_boolean_proc);
    add_procedure("symbol?", is_symbol_proc);
    add_procedure("integer?", is_integer_proc);
    add_procedure("char?", is_char_proc);
    add_procedure("string?", is_string_proc);
    add_procedure("pair?", is_pair_proc);
    add_procedure("procedure?", is_procedure_proc);

    add_procedure("char->integer", char_to_integer_proc);
    add_procedure("integer->char", integer_to_char_proc);
    add_procedure("number->string", number_to_string_proc);
    add_procedure("string->number", string_to_number_proc);
    add_procedure("symbol->string", symbol_to_string_proc);
    add_procedure("string->symbol", string_to_symbol_proc);

    add_procedure("+", add_proc);
    add_procedure("-", sub_proc);
    add_procedure("*", mul_proc);
    add_procedure("quotient", quotient_proc);
    add_procedure("remainder", remainder_proc);
    add_procedure("=", is_number_equal_proc);
    add_procedure("<", is_less_than_proc);
    add_procedure(">", is_greater_than_proc);

    add_procedure("cons", cons_proc);
    add_procedure("car", car_proc);
    add_procedure("cdr", cdr_proc);
    add_procedure("set-car!", set_car_proc);
    add_procedure("set-cdr!", set_cdr_proc);
    add_procedure("list", list_proc);

    add_procedure("eq?", is_eq_proc);

    add_procedure("apply", apply_proc);

    add_procedure("interaction-environment",
                  interaction_environment_proc);
    add_procedure("null-environment", null_environment_proc);
    add_procedure("environment", environment_proc);
    add_procedure("eval", eval_proc);
}
schemeobject *make_environment(void)
{
    schemeobject *env;

    env = setup_environment();
    //printf("The value of initial_env Next is: %p\n", (void *) env->next);

    populate_environment(env);
    //printf("The value of populate_environment Next is: %p\n", (void *) env->next);
    //-->populating env changes its next pointers, because they are added
    return env;
}

//init
void init(void)
{

    //GC
    // create a local variable which points to the first item of the free list (called head).
    //-->init list of objects
    schemeobject *head = NULL;
    head = malloc(sizeof(schemeobject));
    head->mark = 0;
    head->next = NULL;
    long i;
    for (i = 1; i < number_of_objects; ++i)
    {
        push_newschemeobject_to_linked_list(head, 0);
    }
    //printf("variable head is at address: %p\n", (void *)head);

    //init free_list
    free_list = head;
    //init active List
    active_list = NULL;

    //printf("%s", "free_List:"); // %s is format specifier
    //print_next_list(free_list);

    the_empty_list = alloc_object(1);

    the_empty_list->type = THE_EMPTY_LIST;
    //true and false which reader returns as singleton
    false = alloc_object(1);

    false->type = BOOLEAN;
    false->data.boolean.value = 0;

    // add false to root list
    push_pointer_to_existing_schemeobject_to_root_list(false);

    true = alloc_object(1);

    true->type = BOOLEAN;
    true->data.boolean.value = 1;

    // add true to root list
    push_pointer_to_existing_schemeobject_to_root_list(true);

    symbol_table = the_empty_list;
    push_pointer_to_existing_schemeobject_to_root_list(symbol_table);

    quote_symbol = make_symbol("quote");

    //environment
    define_symbol = make_symbol("define");
    //symbol table = empty list
    //refresh rootlist after each make symbol
    set_symbol = make_symbol("set!");
    ok_symbol = make_symbol("ok");
    //if
    if_symbol = make_symbol("if");
    //lambda
    lambda_symbol = make_symbol("lambda");
    //begin
    begin_symbol = make_symbol("begin");
    //cond
    cond_symbol = make_symbol("cond");
    else_symbol = make_symbol("else");
    //let
    let_symbol = make_symbol("let");
    //and or
    and_symbol = make_symbol("and");
    or_symbol = make_symbol("or");

    the_empty_environment = the_empty_list;

    //add empty list to root list only here because now its not equal to symbol_table anymore
    push_pointer_to_existing_schemeobject_to_root_list(the_empty_list);

    the_global_environment = make_environment();

    push_pointer_to_existing_schemeobject_to_root_list(the_global_environment);

    fprintf(stderr, "getRootCount: %d\n", getRootCount());
}
/***************************** READ ******************************/

//check if seperator
char is_delimiter(int c)
{
    return isspace(c) || c == EOF ||
           c == '(' || c == ')' ||
           c == '"' || c == ';';
}

char is_initial(int c)
{
    return isalpha(c) || c == '*' || c == '/' || c == '>' ||
           c == '<' || c == '=' || c == '?' || c == '!';
}

int peek(FILE *in)
{
    int c;
    //get next (in this case first) char of FILE
    c = getc(in);
    //If c does not equal EOF, pushes the character c (reinterpreted as unsigned char) back into the input buffer associated with the stream
    //in such a manner that subsequent read operation from stream will retrieve that character.
    //The external device associated with the stream is not modified.
    ungetc(c, in);
    return c;
}

void eat_whitespace(FILE *in)
{
    int c;

    while ((c = getc(in)) != EOF)
    {
        if (isspace(c))
        {
            continue;
        }
        else if (c == ';')
        { // comments are whitespace also
            while (((c = getc(in)) != EOF) && (c != '\n'))
                ;
            continue;
        }
        ungetc(c, in);
        break;
    }
}

void eat_expected_string(FILE *in, char *str)
{
    int c;

    while (*str != '\0')
    {
        c = getc(in);
        if (c != *str)
        {
            fprintf(stderr, "unexpected character '%c'\n", c);
            exit(1);
        }
        str++;
    }
}

void peek_expected_delimiter(FILE *in)
{
    if (!is_delimiter(peek(in)))
    {
        fprintf(stderr, "character not followed by delimiter\n");
        exit(1);
    }
}

schemeobject *read_character(FILE *in)
{
    int c;

    c = getc(in);
    switch (c)
    {
    case EOF:
        fprintf(stderr, "incomplete character literal\n");
        exit(1);
    case 's':
        if (peek(in) == 'p')
        {
            eat_expected_string(in, "pace");
            peek_expected_delimiter(in);
            return make_char(' ');
        }
        break;
    case 'n':
        if (peek(in) == 'e')
        {
            eat_expected_string(in, "ewline");
            peek_expected_delimiter(in);
            return make_char('\n');
        }
        break;
    }
    peek_expected_delimiter(in);
    return make_char(c);
}

schemeobject *read(FILE *in);

schemeobject *read_pair(FILE *in)
{
    int c;
    schemeobject *car_obj;
    schemeobject *cdr_obj;

    eat_whitespace(in);

    c = getc(in);
    if (c == ')')
    { /* read the empty list */
        return the_empty_list;
    }
    ungetc(c, in);

    car_obj = read(in);

    eat_whitespace(in);

    c = getc(in);
    if (c == '.')
    { /* read improper list */
        c = peek(in);
        if (!is_delimiter(c))
        {
            fprintf(stderr, "dot not followed by delimiter\n");
            exit(1);
        }
        cdr_obj = read(in);
        eat_whitespace(in);
        c = getc(in);
        if (c != ')')
        {
            fprintf(stderr,
                    "where was the trailing right paren?\n");
            exit(1);
        }
        return cons(car_obj, cdr_obj);
    }
    else
    { /* read list */
        ungetc(c, in);
        cdr_obj = read_pair(in);
        return cons(car_obj, cdr_obj);
    }
}
schemeobject *read(FILE *in)
{

    int c;
    short sign = 1;
    long num = 0;
    //for strings
    int i;
#define BUFFER_MAX 1000
    char buffer[BUFFER_MAX];

    //kill whitespaces
    eat_whitespace(in);
    //get next char of input str
    c = getc(in);

    //read boolean or char
    if (c == '#')
    {
        c = getc(in);
        switch (c)
        {
        case 't':
            return true;
        case 'f':
            return false;
        case '\\':
            return read_character(in);
        default:
            fprintf(stderr, "unknown boolean or character literal\n");
            exit(1);
        }
    }

    //falls char ziffer oder: (minus vor einer ziffer)
    else if (isdigit(c) || (c == '-' && (isdigit(peek(in)))))
    {
        /* read a fixnum */
        //falls char ein minus ist setze sign=-1
        if (c == '-')
        {
            sign = -1;
        }
        //falls es eine ziffer ist schreibe zurück in stream
        else
        {
            ungetc(c, in);
        }
        //get all remaining digits which belong to number
        //c is a char in this case!
        while (isdigit(c = getc(in)))
        {
            //alte nummer mal 10 plus neue zahl. Falls neue Zahl = 0 addiere nichts
            //zb:
            //num=31, nächste ziffer ist 0:
            //31*10 = 310 + null = 310
            num = (num * 10) + (c - '0');
        }
        // *-1 if negative
        num *= sign;
        //next char should probably be delimeter after number
        if (is_delimiter(c))
        {
            //write delimeter back to input stream
            ungetc(c, in);
            //return number
            return make_fixnum(num);
        }
        else
        {
            fprintf(stderr, "number not followed by delimiter\n");
            exit(1);
        }
    }
    else if (is_initial(c) ||
             ((c == '+' || c == '-') &&
              is_delimiter(peek(in))))
    { /* read a symbol */
        i = 0;
        while (is_initial(c) || isdigit(c) ||
               c == '+' || c == '-')
        {
            /* subtract 1 to save space for '\0' terminator */
            if (i < BUFFER_MAX - 1)
            {
                buffer[i++] = c;
            }
            else
            {
                fprintf(stderr, "symbol too long. "
                                "Maximum length is %d\n",
                        BUFFER_MAX);
                exit(1);
            }
            c = getc(in);
        }
        if (is_delimiter(c))
        {
            buffer[i] = '\0';
            ungetc(c, in);
            return make_symbol(buffer);
        }
        else
        {
            fprintf(stderr, "symbol not followed by delimiter. "
                            "Found '%c'\n",
                    c);
            exit(1);
        }
    }
    else if (c == '"')
    { /* read a string */
        i = 0;
        while ((c = getc(in)) != '"')
        {
            if (c == '\\')
            {
                c = getc(in);
                if (c == 'n')
                {
                    c = '\n';
                }
            }
            if (c == EOF)
            {
                fprintf(stderr, "non-terminated string literal\n");
                exit(1);
            }
            /* subtract 1 to save space for '\0' terminator */
            if (i < BUFFER_MAX - 1)
            {
                buffer[i++] = c;
            }
            else
            {
                fprintf(stderr,
                        "string too long. Maximum length is %d\n",
                        BUFFER_MAX);
                exit(1);
            }
        }
        buffer[i] = '\0';
        return make_str(buffer);
    }
    else if (c == '(')
    { /* read the empty list or pair */
        return read_pair(in);
    }
    else if (c == '\'')
    { /* read quoted expression */
        return cons(quote_symbol, cons(read(in), the_empty_list));
    }
    //i/io
    else if (c == EOF)
    {
        return NULL;
    }
    else
    {
        fprintf(stderr, "bad input. Unexpected '%c'\n", c);
        exit(1);
    }
    fprintf(stderr, "read illegal state\n");
    exit(1);
}

/*************************** EVALUATE ****************************/

char is_self_evaluating(schemeobject *exp)
{
    return is_boolean(exp) ||
           is_fixnum(exp) ||
           is_character(exp) ||
           is_str(exp);
}

char is_variable(schemeobject *expression)
{
    return is_symbol(expression);
}

char is_tagged_list(schemeobject *expression, schemeobject *tag)
{
    schemeobject *the_car;

    if (is_pair(expression))
    {
        the_car = car(expression);
        return is_symbol(the_car) && (the_car == tag);
    }
    return 0;
}

char is_quoted(schemeobject *expression)
{
    return is_tagged_list(expression, quote_symbol);
}

schemeobject *text_of_quotation(schemeobject *exp)
{
    return cadr(exp);
}

char is_assignment(schemeobject *exp)
{
    return is_tagged_list(exp, set_symbol);
}
schemeobject *assignment_variable(schemeobject *exp)
{
    return car(cdr(exp));
}

schemeobject *assignment_value(schemeobject *exp)
{
    return car(cdr(cdr(exp)));
}

char is_definition(schemeobject *exp)
{
    return is_tagged_list(exp, define_symbol);
}

schemeobject *definition_variable(schemeobject *exp)
{
    if (is_symbol(cadr(exp)))
    {
        return cadr(exp);
    }
    else
    {
        return caadr(exp);
    }
}

schemeobject *make_lambda(schemeobject *parameters, schemeobject *body);

schemeobject *definition_value(schemeobject *exp)
{
    if (is_symbol(cadr(exp)))
    {
        return caddr(exp);
    }
    else
    {
        return make_lambda(cdadr(exp), cddr(exp));
    }
}

schemeobject *make_if(schemeobject *predicate, schemeobject *consequent,
                      schemeobject *alternative)
{
    return cons(if_symbol,
                cons(predicate,
                     cons(consequent,
                          cons(alternative, the_empty_list))));
}

char is_if(schemeobject *expression)
{
    return is_tagged_list(expression, if_symbol);
}

schemeobject *if_predicate(schemeobject *exp)
{
    return cadr(exp);
}

schemeobject *if_consequent(schemeobject *exp)
{
    return caddr(exp);
}

schemeobject *if_alternative(schemeobject *exp)
{
    if (is_the_empty_list(cdddr(exp)))
    {
        return false;
    }
    else
    {
        return cadddr(exp);
    }
}

schemeobject *make_lambda(schemeobject *parameters, schemeobject *body)
{
    return cons(lambda_symbol, cons(parameters, body));
}

char is_lambda(schemeobject *exp)
{
    return is_tagged_list(exp, lambda_symbol);
}

schemeobject *lambda_parameters(schemeobject *exp)
{
    return cadr(exp);
}

schemeobject *lambda_body(schemeobject *exp)
{
    return cddr(exp);
}
//begin
schemeobject *make_begin(schemeobject *exp)
{
    return cons(begin_symbol, exp);
}

char is_begin(schemeobject *exp)
{
    return is_tagged_list(exp, begin_symbol);
}

schemeobject *begin_actions(schemeobject *exp)
{
    return cdr(exp);
}

char is_last_exp(schemeobject *seq)
{
    return is_the_empty_list(cdr(seq));
}

schemeobject *first_exp(schemeobject *seq)
{
    return car(seq);
}

schemeobject *rest_exps(schemeobject *seq)
{
    return cdr(seq);
}

char is_cond(schemeobject *exp)
{
    return is_tagged_list(exp, cond_symbol);
}

schemeobject *cond_clauses(schemeobject *exp)
{
    return cdr(exp);
}

schemeobject *cond_predicate(schemeobject *clause)
{
    return car(clause);
}

schemeobject *cond_actions(schemeobject *clause)
{
    return cdr(clause);
}

char is_cond_else_clause(schemeobject *clause)
{
    return cond_predicate(clause) == else_symbol;
}

schemeobject *sequence_to_exp(schemeobject *seq)
{
    if (is_the_empty_list(seq))
    {
        return seq;
    }
    else if (is_last_exp(seq))
    {
        return first_exp(seq);
    }
    else
    {
        return make_begin(seq);
    }
}

schemeobject *expand_clauses(schemeobject *clauses)
{
    schemeobject *first;
    schemeobject *rest;

    if (is_the_empty_list(clauses))
    {
        return false;
    }
    else
    {
        first = car(clauses);
        rest = cdr(clauses);
        if (is_cond_else_clause(first))
        {
            if (is_the_empty_list(rest))
            {
                return sequence_to_exp(cond_actions(first));
            }
            else
            {
                fprintf(stderr, "else clause isn't last cond->if");
                exit(1);
            }
        }
        else
        {
            return make_if(cond_predicate(first),
                           sequence_to_exp(cond_actions(first)),
                           expand_clauses(rest));
        }
    }
}

schemeobject *cond_to_if(schemeobject *exp)
{
    return expand_clauses(cond_clauses(exp));
}
//let
schemeobject *make_application(schemeobject *operator, schemeobject *operands)
{
    return cons(operator, operands);
}

char is_application(schemeobject *exp)
{
    return is_pair(exp);
}

schemeobject *operator(schemeobject *exp)
{
    return car(exp);
}

schemeobject *operands(schemeobject *exp)
{
    return cdr(exp);
}

char is_no_operands(schemeobject *ops)
{
    return is_the_empty_list(ops);
}

schemeobject *first_operand(schemeobject *ops)
{
    return car(ops);
}

schemeobject *rest_operands(schemeobject *ops)
{
    return cdr(ops);
}

//let
char is_let(schemeobject *exp)
{
    return is_tagged_list(exp, let_symbol);
}

schemeobject *let_bindings(schemeobject *exp)
{
    return cadr(exp);
}

schemeobject *let_body(schemeobject *exp)
{
    return cddr(exp);
}

schemeobject *binding_parameter(schemeobject *binding)
{
    return car(binding);
}

schemeobject *binding_argument(schemeobject *binding)
{
    return cadr(binding);
}

schemeobject *bindings_parameters(schemeobject *bindings)
{
    return is_the_empty_list(bindings) ? the_empty_list : cons(binding_parameter(car(bindings)), bindings_parameters(cdr(bindings)));
}

schemeobject *bindings_arguments(schemeobject *bindings)
{
    return is_the_empty_list(bindings) ? the_empty_list : cons(binding_argument(car(bindings)), bindings_arguments(cdr(bindings)));
}

schemeobject *let_parameters(schemeobject *exp)
{
    return bindings_parameters(let_bindings(exp));
}

schemeobject *let_arguments(schemeobject *exp)
{
    return bindings_arguments(let_bindings(exp));
}

schemeobject *let_to_application(schemeobject *exp)
{
    return make_application(
        make_lambda(let_parameters(exp),
                    let_body(exp)),
        let_arguments(exp));
}
//and or
char is_and(schemeobject *exp)
{
    return is_tagged_list(exp, and_symbol);
}

schemeobject *and_tests(schemeobject *exp)
{
    return cdr(exp);
}

char is_or(schemeobject *exp)
{
    return is_tagged_list(exp, or_symbol);
}

schemeobject *or_tests(schemeobject *exp)
{
    return cdr(exp);
}

//apply
schemeobject *apply_operator(schemeobject *arguments)
{
    return car(arguments);
}

schemeobject *prepare_apply_operands(schemeobject *arguments)
{
    if (is_the_empty_list(cdr(arguments)))
    {
        return car(arguments);
    }
    else
    {
        return cons(car(arguments),
                    prepare_apply_operands(cdr(arguments)));
    }
}

schemeobject *apply_operands(schemeobject *arguments)
{
    return prepare_apply_operands(cdr(arguments));
}

//eval
schemeobject *eval_expression(schemeobject *arguments)
{
    return car(arguments);
}

schemeobject *eval_environment(schemeobject *arguments)
{
    return cadr(arguments);
}

schemeobject *eval(schemeobject *exp, schemeobject *env);

//simple procedures
schemeobject *list_of_values(schemeobject *exps, schemeobject *env)
{
    if (is_no_operands(exps))
    {
        return the_empty_list;
    }
    else
    {
        return cons(eval(first_operand(exps), env),
                    list_of_values(rest_operands(exps), env));
    }
}
//
schemeobject *eval_assignment(schemeobject *exp, schemeobject *env)
{
    set_variable_value(assignment_variable(exp),
                       eval(assignment_value(exp), env),
                       env);
    return ok_symbol;
}

schemeobject *eval_definition(schemeobject *exp, schemeobject *env)
{
    define_variable(definition_variable(exp),
                    eval(definition_value(exp), env),
                    env);
    return ok_symbol;
}

schemeobject *eval(schemeobject *exp, schemeobject *env)
{
    schemeobject *procedure;
    schemeobject *arguments;
    schemeobject *result;
//recursive go through tailcalls
tailcall:
    if (is_self_evaluating(exp))
    {
        return exp;
    }
    else if (is_variable(exp))
    {
        return lookup_variable_value(exp, env);
    }
    else if (is_quoted(exp))
    {
        return text_of_quotation(exp);
    }
    else if (is_assignment(exp))
    {
        return eval_assignment(exp, env);
    }
    else if (is_definition(exp))
    {
        return eval_definition(exp, env);
    }
    else if (is_if(exp))
    {
        exp = is_true(eval(if_predicate(exp), env)) ? if_consequent(exp) : if_alternative(exp);
        goto tailcall;
    }
    else if (is_lambda(exp))
    {
        return make_compound_proc(lambda_parameters(exp),
                                  lambda_body(exp),
                                  env);
    }
    else if (is_begin(exp))
    {
        exp = begin_actions(exp);
        while (!is_last_exp(exp))
        {
            eval(first_exp(exp), env);
            exp = rest_exps(exp);
        }
        exp = first_exp(exp);
        goto tailcall;
    }
    else if (is_cond(exp))
    {
        exp = cond_to_if(exp);
        goto tailcall;
    }
    else if (is_let(exp))
    {
        exp = let_to_application(exp);
        goto tailcall;
    }
    else if (is_and(exp))
    {
        exp = and_tests(exp);
        if (is_the_empty_list(exp))
        {
            return true;
        }
        while (!is_last_exp(exp))
        {
            result = eval(first_exp(exp), env);
            if (is_false(result))
            {
                return result;
            }
            exp = rest_exps(exp);
        }
        exp = first_exp(exp);
        goto tailcall;
    }
    else if (is_or(exp))
    {
        exp = or_tests(exp);
        if (is_the_empty_list(exp))
        {
            return false;
        }
        while (!is_last_exp(exp))
        {
            result = eval(first_exp(exp), env);
            if (is_true(result))
            {
                return result;
            }
            exp = rest_exps(exp);
        }
        exp = first_exp(exp);
        goto tailcall;
    }
    else if (is_application(exp))
    {
        procedure = eval(operator(exp), env);
        arguments = list_of_values(operands(exp), env);

        /* handle eval specially for tail call requirement */
        if (is_primitive_proc(procedure) &&
            procedure->data.primitive_proc.fn == eval_proc)
        {
            exp = eval_expression(arguments);
            env = eval_environment(arguments);
            goto tailcall;
        }

        /* handle apply specially for tailcall requirement */
        if (is_primitive_proc(procedure) &&
            procedure->data.primitive_proc.fn == apply_proc)
        {
            procedure = apply_operator(arguments);
            arguments = apply_operands(arguments);
        }

        if (is_primitive_proc(procedure))
        {
            return (procedure->data.primitive_proc.fn)(arguments);
        }
        else if (is_compound_proc(procedure))
        {
            //fprintf(stderr, "is_compound_proc\n");
            env = init_environment(
                procedure->data.compound_proc.parameters,
                arguments,
                procedure->data.compound_proc.env);
            exp = make_begin(procedure->data.compound_proc.body);
            //goto tailcall: expression now is  sliced through,
            //own small env
            goto tailcall;
        }
        else
        {
            fprintf(stderr, "unknown procedure type\n");
            exit(1);
        }
    }
    else
    {
        fprintf(stderr, "cannot eval unknown expression type\n");
        exit(1);
    }
    fprintf(stderr, "eval illegal state\n");
    exit(1);
}

/**************************** PRINT ******************************/
void write_pair(FILE *out, schemeobject *pair)
{
    schemeobject *car_obj;
    schemeobject *cdr_obj;

    car_obj = car(pair);
    cdr_obj = cdr(pair);
    write(out, car_obj);
    if (cdr_obj->type == PAIR)
    {
        fprintf(out, " ");
        write_pair(out, cdr_obj);
    }
    else if (cdr_obj->type == THE_EMPTY_LIST)
    {
        return;
    }
    else
    {
        fprintf(out, " . ");
        write(out, cdr_obj);
    }
}

void write(FILE *out, schemeobject *obj)
{
    char c;
    char *str;

    switch (obj->type)
    {
    case THE_EMPTY_LIST:
        fprintf(out, "()");
        break;
    case FIXNUM:
        //long signed
        fprintf(out, "%ld", obj->data.fixnum.value);
        break;
    case BOOLEAN:
        //einzelnes zeichen
        fprintf(out, "#%c", is_false(obj) ? 'f' : 't');
        break;
    case SYMBOL:
        fprintf(out, "%s", obj->data.symbol.value);
        break;

    case CHARACTER:
        c = obj->data.character.value;
        fprintf(out, "#\\");
        switch (c)
        {
        case '\n':
            fprintf(out, "newline");
            break;
        case ' ':
            fprintf(out, "space");
            break;
        default:
            putc(c, out);
        }
        break;
    case STRING:
        str = obj->data.string.value;
        putchar('"');
        while (*str != '\0')
        {
            switch (*str)
            {
            //esacpe c escape chars
            case '\n':
                fprintf(out, "\\n");
                break;
            case '\\':
                fprintf(out, "\\\\");
                break;
            case '"':
                fprintf(out, "\\\"");
                break;
            default:
                putc(*str, out);
            }
            str++;
        }
        putchar('"');
        break;
    case PAIR:
        fprintf(out, "(");
        write_pair(out, obj);
        fprintf(out, ")");
        break;
    case PRIMITIVE_PROC:
        fprintf(out, "#<primitive-procedure>");
        break;
    case COMPOUND_PROC:
        fprintf(out, "#<procedure>");
        break;
    default:
        fprintf(stderr, "cannot write unknown type\n");
        exit(1);
    }
}

/***************************** REPL ******************************/
//to check stack & heap direction
void sub(int *a)
{
    int b;

    if (&b > a)
    {
        printf("Stack grows up. \nHeap grows down.\n");
    }
    else
    {
        printf("Stack grows down.\nHeap grows up.\n");
    }
}
int main(int argc, char **argv)
{
    char z[100] = "teststring\n";
    printf("%s", z);
    //to check stack & heap direction
    int a;
    sub(&a);

    schemeobject *exp;

    printf("Welcome to Bootstrap Scheme. "
           "Use ctrl-c to exit.\n");
    //printf("test:\\n \\\" \n");

    init();
    load_proc_from_str("stdlib.scm");
    printf("stdlib loaded.\n");
    print_objects_status();
    while (1)
    {
        printf("> ");
        exp = read(stdin);
        if (exp == NULL)
        {
            break;
        }
        write(stdout, eval(exp, the_global_environment));

        printf("\n");
        print_objects_status();
    }
    //to make compiler happy
    return 0;
}
