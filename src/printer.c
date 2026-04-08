#include "printer.h"

#include <stdio.h>

static void print_csv_string(const char *value);

void print_user_row(const char values[USER_COLUMN_COUNT][128]) {
    if (values == NULL) {
        return;
    }

    printf("%s,", values[0]);
    print_csv_string(values[1]);
    putchar(',');
    print_csv_string(values[2]);
    printf(",%s,", values[3]);
    print_csv_string(values[4]);
    putchar(',');
    print_csv_string(values[5]);
    putchar('\n');
}

void print_select_header(void) {
    puts("id,username,name,age,phone,email");
}

void print_rows_selected(int count) {
    printf("%d rows selected\n", count);
}

void print_message(const char *message) {
    if (message != NULL) {
        puts(message);
    }
}

void print_error(const char *message) {
    if (message != NULL) {
        puts(message);
    }
}

static void print_csv_string(const char *value) {
    const char *cursor = value;

    if (value == NULL) {
        fputs("\"\"", stdout);
        return;
    }

    putchar('"');
    while (*cursor != '\0') {
        if (*cursor == '"') {
            putchar('"');
        }
        putchar(*cursor);
        cursor++;
    }
    putchar('"');
}
