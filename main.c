#include "common.c"

typedef enum {
    TOKEN_IDENTIFIER = 128,
    TOKEN_NUMBER,
    TOKEN_LENGTH
} TokenKind;

typedef struct {
    TokenKind kind;
    union {
        uint64_t int_val;
        char *str_val;
    };
} Token;

typedef struct {
    InputBuf input;
    Token current_token;
    StringTable intern_table;
} Parser;

void next_token(Parser *parser) {
    InputBuf *input = &parser->input;
    while (isspace(*input->buf_pos)) {
        input->buf_pos++;
    }
    switch (*input->buf_pos) {
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
        case '8': case '9': {
            parser->current_token.int_val = *input->buf_pos++ - '0';
            while (isdigit(*input->buf_pos)) {
                parser->current_token.int_val *= 10;
                parser->current_token.int_val += *input->buf_pos - '0';
                input->buf_pos++;
            }
            parser->current_token.kind = TOKEN_NUMBER;
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
            parser->current_token.str_val = intern_str_range(&parser->intern_table, start, end);
            parser->current_token.kind = TOKEN_IDENTIFIER;
            break;
        }
        case 0:
            return;
        default:
            parser->current_token.kind = *input->buf_pos++;
            break;
    }
}

char *token_kind_to_str[TOKEN_LENGTH] = {
    [','] = ",",
    ['('] = "(",
    [')'] = ")",
    [TOKEN_IDENTIFIER] = "id",
    [TOKEN_NUMBER] = "number"
};

Token expect_token_kind_and_eat(Parser *parser, TokenKind kind) {
    TokenKind k = parser->current_token.kind;
    if (k != kind) {
        syntax_error("Error: expected token %s got %s\n", token_kind_to_str[kind], token_kind_to_str[k]);
    } else {
        Token eated = parser->current_token;
        next_token(parser);
        return eated;
    }
}

Token expect_token_keyword_and_eat(Parser *parser, char *keyword) {
    char *k = parser->current_token.str_val;
    if (k != keyword) {
        syntax_error("Error: expected keyword %s got %s\n", keyword, k);
    } else {
        Token eated = parser->current_token;
        next_token(parser);
        return eated;
    }
}

bool match_token_kind_and_eat(Parser *parser, TokenKind kind) {
    TokenKind k = parser->current_token.kind;
    next_token(parser);
    return k == kind;
}

char *keyword_create = "create";
char *keyword_table = "table";

#define intern(p, x) p##_##x = intern_str(intern_table, p##_##x)

void init_keywords(StringTable *intern_table) {
    intern(keyword, create);
    intern(keyword, table);
}

char *typename_int = "int";
char *typename_integer = "integer";

char *typename_char = "char";
char *typename_varchar = "varchar";

void init_typenames(StringTable *intern_table) {
    intern(typename, int);
    intern(typename, integer);
    
    intern(typename, char);
    intern(typename, varchar);
}

char *metacommand_tables = "tables";
char *metacommand_help = "help";
char *metacommand_exit = "exit";

void init_metacommands(StringTable *intern_table) {
    intern(metacommand, tables);
    intern(metacommand, help);
    intern(metacommand, exit);
}

Parser *init_parser() {
    Parser *parser = (Parser *)calloc(1, sizeof(Parser));
    init_keywords(&parser->intern_table);
    init_typenames(&parser->intern_table);
    init_metacommands(&parser->intern_table);
    return parser;
}

typedef struct {
    char *name;
    size_t size;
} Type;

typedef struct {
    char *name;
    Type *type;
} ColumnSpec;

Type *parse_column_type(Parser *parser) {
    char *str = parser->current_token.str_val;
    Type *type;
    next_token(parser);
    if (str == typename_int || str == typename_integer) {
        type = (Type *)malloc(sizeof(Type));
        type->name = str;
        type->size = sizeof(int);
    } else if (str == typename_char || str == typename_varchar) {
        expect_token_kind_and_eat(parser, '(');
        Token type_size_token = expect_token_kind_and_eat(parser, TOKEN_NUMBER);
        uint64_t type_size = type_size_token.int_val;
        expect_token_kind_and_eat(parser, ')');
        type = (Type *)malloc(sizeof(Type));
        type->name = str;
        type->size = type_size * sizeof(char);
    }
    return type;
}

ColumnSpec *parse_column_specifier(Parser *parser) {
    Token column_name_token = expect_token_kind_and_eat(parser, TOKEN_IDENTIFIER);
    char *column_name = column_name_token.str_val;
    Type *column_type = parse_column_type(parser);
    ColumnSpec *spec = (ColumnSpec *)malloc(sizeof(ColumnSpec));
    spec->name = column_name;
    spec->type = column_type;
    return spec;
}

ColumnSpec **parse_column_specifier_list(Parser *parser) {
    ColumnSpec **specs = 0;
    ColumnSpec *spec = parse_column_specifier(parser);
    buf_push(specs, spec);
    while (match_token_kind_and_eat(parser, ',')) {
        spec = parse_column_specifier(parser);
        buf_push(specs, spec);
    }
    return specs;
}

void parse_create_clause(Parser *parser) {
    expect_token_keyword_and_eat(parser, keyword_create);
    expect_token_keyword_and_eat(parser, keyword_table);
    Token table_name_token = expect_token_kind_and_eat(parser, TOKEN_IDENTIFIER);
    char *table_name = table_name_token.str_val;
    expect_token_kind_and_eat(parser, '(');
    ColumnSpec **columns = parse_column_specifier_list(parser);
    expect_token_kind_and_eat(parser, ')');
}

void execute_meta_command(Parser *parser) {
    next_token(parser);
    Token meta_command_token = expect_token_kind_and_eat(parser, TOKEN_IDENTIFIER);
    char *meta_command = meta_command_token.str_val;
    if (meta_command == metacommand_tables) {
        printf("Tables...\n");
    } else if (meta_command == metacommand_help) {
        printf("Help...\n");
    } else if (meta_command == metacommand_exit) {
        printf("Exiting...\n");
        exit(0);
    } else {
        printf("Bad metacommand.\n");
    }
}

void parse_command(Parser *parser) {
    next_token(parser);
    switch (parser->current_token.kind) {
        case TOKEN_IDENTIFIER:
            if (parser->current_token.str_val == keyword_create) {
                parse_create_clause(parser);
            }
            break;
        case '.':
            execute_meta_command(parser);
            break;
        default:
            printf("Error\n");
    }
}

int main(int argc, char *argv[]) {
    table_test();
    Parser *parser = init_parser();
    while (true) {
        print_prompt();
        read_input(&parser->input);
        parse_command(parser); 
    }
    return 0;
}