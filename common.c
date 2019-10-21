#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>

typedef struct {
    size_t cap;
    size_t len;
    char buf[0];
} BufHdr;

#define max(a, b) ((a) > (b) ? (a) : (b))

#define buf__hdr(b) ((BufHdr *)((char *)b - offsetof(BufHdr, buf)))
#define buf__fits(b, n) (buf_len(b) + (n) <= buf_cap(b))
#define buf__fit(b, n) (buf__fits(b, n) ? 0 : ((b) = buf__grow((b), buf_len(b) + (n), sizeof(*(b)))))

#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_push(b, x) (buf__fit(b, 1), (b)[buf_len(b)] = (x), buf__hdr(b)->len++)

void *buf__grow(void *buf, size_t new_len, size_t elem_size) {
    size_t new_cap = max(2*buf_cap(buf)+1, new_len);
    size_t new_size = new_cap * elem_size + offsetof(BufHdr, buf);
    BufHdr *new_hdr;
    if (buf) {
        new_hdr = (BufHdr *)realloc(buf - offsetof(BufHdr, buf), new_size);
    } else {
        new_hdr = (BufHdr *)malloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

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
    size_t input_len = getline(&input->buf, &input->buf_len, stdin);
    input->buf_pos = input->buf;
    // Ignore trailing newline character
    input->input_len = input_len-1;
    input->buf[input_len-1] = 0;
}

enum {
    MAX_STRING_TABLE_SIZE = 128,
    LINEAR_PROBE_INDEX = 1
};

typedef struct {
    bool occupied;
    char *val;
} TableEntry;

typedef struct {
    size_t len;
    TableEntry entries[MAX_STRING_TABLE_SIZE];
} StringTable;

uint32_t hash_str(char *str) {
    assert(str);
    uint32_t hash = 0;
    while (*str) {
        hash += *str++;
    }
    return hash % MAX_STRING_TABLE_SIZE;
}

uint32_t hash_str_range(char *start, char *end) {
    assert(start);
    assert(end);
    assert(start < end);
    uint32_t hash = 0;
    while (start != end) {
        hash += *start++;
    }
    return hash % MAX_STRING_TABLE_SIZE;
}

TableEntry *table_find_next_free_entry(StringTable *table, uint32_t start) {
    while (table->entries[start].occupied) {
        start += LINEAR_PROBE_INDEX;
        if (start >= MAX_STRING_TABLE_SIZE) {
            start %= MAX_STRING_TABLE_SIZE;
        }
    }
    return &table->entries[start];
}

void table_put_str(StringTable *table, char *str) {
    assert(table);
    assert(table->len < MAX_STRING_TABLE_SIZE);
    uint32_t hash = hash_str(str);
    TableEntry *entry = &table->entries[hash];
    if (entry->occupied) {
        entry = table_find_next_free_entry(table, hash + LINEAR_PROBE_INDEX);
    }
    entry->occupied = 1;
    entry->val = str;
    table->len++;
}

char *table_get_str(StringTable *table, char *str) {
    assert(table);
    uint32_t hash = hash_str(str);
    TableEntry *entry = &table->entries[hash];
    if (!entry->occupied) {
        return 0;
    }
    if (strcmp(entry->val, str) == 0) {
        return entry->val;
    } else {
        uint32_t num_comp = 0;
        while (num_comp < MAX_STRING_TABLE_SIZE) {
            hash += LINEAR_PROBE_INDEX;
            if (hash >= MAX_STRING_TABLE_SIZE) {
                hash %= MAX_STRING_TABLE_SIZE;
            }
            entry = &table->entries[hash];
            if (entry->occupied && strcmp(entry->val, str) == 0) {
                return entry->val;
            }
            num_comp++;
        }
    }
    return 0;
}

char *table_get_str_range(StringTable *table, char *start, char *end) {
    assert(table);
    uint32_t hash = hash_str_range(start, end);
    TableEntry *entry = &table->entries[hash];
    size_t str_len = end - start;
    if (!entry->occupied) {
        return 0;
    }
    if (strncmp(entry->val, start, str_len) == 0) {
        return entry->val;
    } else {
        uint32_t num_comp = 0;
        while (num_comp < MAX_STRING_TABLE_SIZE) {
            hash += LINEAR_PROBE_INDEX;
            if (hash >= MAX_STRING_TABLE_SIZE) {
                hash %= MAX_STRING_TABLE_SIZE;
            }
            entry = &table->entries[hash];
            if (entry->occupied && strncmp(entry->val, start, str_len) == 0) {
                return entry->val;
            }
            num_comp++;
        }
    }
    return 0;
}

void table_test() {
    StringTable *table = (StringTable *)calloc(1, sizeof(StringTable));
    assert(table);
    assert(table->len == 0);
    char *k = "Hello";
    table_put_str(table, k);
    assert(table->len == 1);
    assert(k == "Hello");
    char *tk = table_get_str(table, k);
    assert(tk == k);
    char *i = "World";
    table_put_str(table, i);
    assert(table->len == 2);
    char *ti = table_get_str(table, i);
    assert(ti == i);
    char *j = "ab";
    char *w = "ba";
    assert(hash_str(j) == hash_str(w));
    table_put_str(table, j);
    table_put_str(table, w);
    char *tj = table_get_str(table, j);
    char *tw = table_get_str(table, w);
    assert(tj == j);
    assert(tw == w);
    assert(tj != tw);
    free(table);
}

char *intern_str(StringTable *table, char *str) {
    char *s = table_get_str(table, str);
    if (s) {
        return s;
    }
    size_t str_len = strlen(str);
    s = (char *)malloc(str_len);
    strncpy(s, str, str_len);
    s[str_len] = 0;
    table_put_str(table, s);
    return s;
}

char *intern_str_range(StringTable *table, char *start, char *end) {
    char *s = table_get_str_range(table, start, end);
    if (s) {
        return s;
    }
    size_t str_len = end - start;
    s = (char *)malloc(str_len);
    strncpy(s, start, str_len);
    s[str_len] = 0;
    table_put_str(table, s);
    return s;
}

void syntax_error(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}