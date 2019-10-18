#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

typedef struct {
    size_t buf_len;
    size_t input_len;
    char *buf;
    char *buf_pos;
} InputBuf;

void print_prompt() {
    printf("db > ");
}

void read_input(InputBuf *input) {
    size_t input_len = getline(&(input->buf), &input->buf_len, stdin);
    input->buf_pos = input->buf;
    // Ignore trailing newline character
    input->input_len = input_len-1;
    input->buf[input_len-1] = 0;
}

enum {
    MAX_INTERN_SIZE = 128
};

typedef struct {
    size_t len;
    char *buf[MAX_INTERN_SIZE];
} InternTable;

char *intern_str_range(InternTable *table, char *start, char *end) {
    size_t str_len = end - start;
    for (int i = 0; i < table->len; ++i) {
        if (strncmp(table->buf[i], start, str_len) == 0) {
            return table->buf[i];
        }
    }
    char *dest = (char *)malloc(str_len);
    dest[str_len] = 0;
    return table->buf[table->len++] = strncpy(dest, start, str_len);
}

char *intern_str(InternTable *table, char *str) {
    size_t str_len = strlen(str);
    return intern_str_range(table, str, str + str_len);
}

char *keyword_create = "create";
char *keyword_table = "table";

#define intern(x) keyword_##x = intern_str(table, keyword_##x)

void init_keywords(InternTable *table) {
    intern(create);
    intern(table);

    #undef intern
}

typedef enum {
    TOKEN_IDENTIFIER = 128,
    TOKEN_INT
} TokenKind;

typedef struct {
    TokenKind kind;
    union {
        uint64_t int_val;
        char *str_val;
    };
} Token;

Token next_token(InputBuf *input, InternTable *table) {
    Token token;
    while (isspace(*input->buf_pos)) {
        input->buf_pos++;
    }
    switch (*input->buf_pos) {
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
        case '8': case '9': {
            token.int_val = 0;
            while (isdigit(*input->buf_pos)) {
                token.int_val *= 10;
                token.int_val += *input->buf_pos - '0';
                input->buf_pos++;
            }
            token.kind = TOKEN_INT;
            break;
        }
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
        case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
        case 's': case 't': case 'u': case 'v': case 'w': case 'x':
        case 'y': case 'z':

        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
        case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
        case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
        case 'Y': case 'Z': {
            char *start = input->buf_pos++;
            while (isalnum(*input->buf_pos)) {
                input->buf_pos++;
            }
            char *end = input->buf_pos;
            token.str_val = intern_str_range(table, start, end);
            token.kind = TOKEN_IDENTIFIER;
            break;
        }
        default:
            token.kind = *input->buf_pos++;
    }
    return token;
}

int main(int argc, char *argv[]) {
    InputBuf input;
    InternTable table;
    init_keywords(&table);
    while (true) {
        print_prompt();
        read_input(&input);
        while (*input.buf_pos) {
            Token token = next_token(&input, &table);
            switch (token.kind) {
                case TOKEN_INT:
                    printf("%llu\n", token.int_val);
                    break;
                case TOKEN_IDENTIFIER:
                    printf("%s\n", token.str_val);
                    break;
                default:
                    printf("%c\n", token.kind);
                    break;
            }
        }    
    }
    return 0;
}