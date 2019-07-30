#include <stdlib.h> // standard
#include <stdio.h> //input output
#include <string.h> //string
#include <ctype.h> //testing and mapping characters

/**************************** MODEL ******************************/

//define enum with name "object_type" with only option FIXNUM as choice
typedef enum {BOOLEAN, FIXNUM, CHARACTER, STRING} object_type;
    
typedef struct object {
    object_type type;
    //A union is a type consisting of a sequence of members whose storage overlaps
    //union is named data
    union {
        //A struct is a type consisting of a sequence of members whose storage is allocated in an ordered sequence
        //struct is named "fixnum"
         struct {
            char value;
        } boolean;
        struct {
            long value;
        } fixnum;
                struct {
            char value;
        } character;
        struct {
            char *value;
        } string;      
    } data;
} object;


/* no Garbage Ccollector so far, so truely "unlimited extent" */


object *alloc_object(void) {
    object *obj;

    obj = malloc(sizeof(object));
    if (obj == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    return obj;
}
//*****************boolean//
object *false;
object *true;

char is_boolean(object *obj) {
    return obj->type == BOOLEAN;
}

char is_false(object *obj) {
    return obj == false;
}

char is_true(object *obj) {
    return !is_false(obj);
}
///**fixnum
object *make_fixnum(long value) {
    object *obj;

    obj = alloc_object();
    obj->type = FIXNUM;
    obj->data.fixnum.value = value;
    return obj;
}

char is_fixnum(object *obj) {
    return obj->type == FIXNUM;
}

object *make_character(char value) {
    object *obj;

    obj = alloc_object();
    obj->type = CHARACTER;
    obj->data.character.value = value;
    return obj;
}

object *make_string(char *value) {
    object *obj;

    obj = alloc_object();
    obj->type = STRING;
    obj->data.string.value = malloc(strlen(value) + 1);
    if (obj->data.string.value == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    strcpy(obj->data.string.value, value);
    return obj;
}

char is_string(object *obj) {
    return obj->type == STRING;
}

char is_character(object *obj) {
    return obj->type == CHARACTER;
}

//init true and false which reader returns as singleton
void init(void) {
    false = alloc_object();
    false->type = BOOLEAN;
    false->data.boolean.value = 0;

    true = alloc_object();
    true->type = BOOLEAN;
    true->data.boolean.value = 1;
}
/***************************** READ ******************************/

//check if seperator
char is_delimiter(int c) {
    return isspace(c) || c == EOF ||
           c == '('   || c == ')' ||
           c == '"'   || c == ';';
}


int peek(FILE *in) {
    int c;
    //get next (in this case first) char of FILE
    c = getc(in);
    //If c does not equal EOF, pushes the character c (reinterpreted as unsigned char) back into the input buffer associated with the stream
    //in such a manner that subsequent read operation from stream will retrieve that character. 
    //The external device associated with the stream is not modified.
    ungetc(c, in);
    return c;
}

void eat_whitespace(FILE *in) {
    int c;
    
    while ((c = getc(in)) != EOF) {
        if (isspace(c)) {
            continue;
        }
        else if (c == ';') { /* comments are whitespace also */
            while (((c = getc(in)) != EOF) && (c != '\n'));
            continue;
        }
        ungetc(c, in);
        break;
    }
}

void eat_expected_string(FILE *in, char *str) {
    int c;

    while (*str != '\0') {
        c = getc(in);
        if (c != *str) {
            fprintf(stderr, "unexpected character '%c'\n", c);
            exit(1);
        }
        str++;
    }
}

void peek_expected_delimiter(FILE *in) {
    if (!is_delimiter(peek(in))) {
        fprintf(stderr, "character not followed by delimiter\n");
        exit(1);
    }
}

object *read_character(FILE *in) {
    int c;

    c = getc(in);
    switch (c) {
        case EOF:
            fprintf(stderr, "incomplete character literal\n");
            exit(1);
        case 's':
            if (peek(in) == 'p') {
                eat_expected_string(in, "pace");
                peek_expected_delimiter(in);
                return make_character(' ');
            }
            break;
        case 'n':
            if (peek(in) == 'e') {
                eat_expected_string(in, "ewline");
                peek_expected_delimiter(in);
                return make_character('\n');
            }
            break;
    }
    peek_expected_delimiter(in);
    return make_character(c);
}

object *read(FILE *in) {

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
    if (c == '#') { 
        c = getc(in);
        switch (c) {
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
    else if (isdigit(c) || (c == '-' && (isdigit(peek(in))))) {
        /* read a fixnum */
        //falls char ein minus ist setze sign=-1
        if (c == '-') {
            sign = -1;
        }
        //falls es eine ziffer ist schreibe zurück in stream
        else {
            ungetc(c, in);
        }
        //get all remaining digits which belong to number
        //c is a char in this case!
        while (isdigit(c = getc(in))) {
            //alte nummer mal 10 plus neue zahl. Falls neue Zahl = 0 addiere nichts
            //zb:
            //num=31, nächste ziffer ist 0:
            //31*10 = 310 + null = 310
            num = (num * 10) + (c - '0');
        }
        // *-1 if negative
        num *= sign;
        //next char should probably be delimeter after number
        if (is_delimiter(c)) {
            //write delimeter back to input stream
            ungetc(c, in);
            //return number
            return make_fixnum(num);
        }
        else {
            fprintf(stderr, "number not followed by delimiter\n");
            exit(1);
        }
    }
    else if (c == '"') { /* read a string */
        i = 0;
        while ((c = getc(in)) != '"') {
            if (c == '\\') {
                c = getc(in);
                if (c == 'n') {
                    c = '\n';
                }
            }
            if (c == EOF) {
                fprintf(stderr, "non-terminated string literal\n");
                exit(1);
            }
            /* subtract 1 to save space for '\0' terminator */
            if (i < BUFFER_MAX - 1) {
                buffer[i++] = c;
            }
            else {
                fprintf(stderr, 
                        "string too long. Maximum length is %d\n",
                        BUFFER_MAX);
                exit(1);
            }
        }
        buffer[i] = '\0';
        return make_string(buffer);
    }
    else {
        fprintf(stderr, "bad input. Unexpected '%c'\n", c);
        exit(1);
    }
    fprintf(stderr, "read illegal state\n");
    exit(1);
}

/*************************** EVALUATE ****************************/

/* until we have lists and symbols just echo */
object *eval(object *exp) {
    return exp;
}

/**************************** PRINT ******************************/

void write(object *obj) {
    char c;
    char *str;

    switch (obj->type) {
        case BOOLEAN:
            //einzelnes zeichen
            printf("#%c", is_false(obj) ? 'f' : 't');
            break;
        case FIXNUM:
            //long signed
            printf("%ld", obj->data.fixnum.value);
            break;
        case CHARACTER:
            c = obj->data.character.value;
            printf("#\\");
            switch (c) {
                case '\n':
                    printf("newline");
                    break;
                case ' ':
                    printf("space");
                    break;
                default:
                    putchar(c);
            }
            break;
        case STRING:
            str = obj->data.string.value;
            putchar('"');
            while (*str != '\0') {
                switch (*str) {
                    case '\n':
                        printf("\\n");
                        break;
                    case '\\':
                        printf("\\\\");
                        break;
                    case '"':
                        printf("\\\"");
                        break;
                    default:
                        putchar(*str);
                }
                str++;
            }
            putchar('"');
            break;
        default:
            fprintf(stderr, "cannot write unknown type\n");
            exit(1);
    }
}

/***************************** REPL ******************************/

int main(void) {

    printf("Welcome to Bootstrap Scheme. "
           "Use ctrl-c to exit.\n");

    init();
    
    while (1) {
        printf("> ");
        write(eval(read(stdin)));
        printf("\n");
    }
    //to make compiler happy
    return 0;
}
