#include "printer.h"

#include <stdio.h>

static void print_csv_string(const char *value);

void print_user_row(char *const values[USER_COLUMN_COUNT]) {
    /* SELECT 결과를 콘솔에 다시 CSV 형태로 보여준다. */
    if (values == NULL) {
        return;
    }

    /* 숫자 칼럼은 그대로,
       문자열 칼럼은 따옴표 포함 CSV 스타일로 출력한다. */
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
    /* 결과를 읽기 쉽게 하기 위해 컬럼 이름을 먼저 출력한다. */
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
    /* 출력도 CSV처럼 보이게 문자열은 큰따옴표로 감싼다. */
    const char *cursor = value;

    /* NULL이면 빈 문자열처럼 ""를 출력한다. */
    if (value == NULL) {
        fputs("\"\"", stdout);
        return;
    }

    /* 문자열 시작 큰따옴표 */
    putchar('"');
    while (*cursor != '\0') {
        /* 화면 출력에서도 따옴표는 두 번 써서 CSV 모양을 유지한다. */
        if (*cursor == '"') {
            putchar('"');
        }
        putchar(*cursor);
        cursor++;
    }
    /* 문자열 끝 큰따옴표 */
    putchar('"');
}
