#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btree.h"
#include "utils.h"

static int append_field_separator(FILE *fp, int index);
static int ensure_user_index_loaded(void);
static int reset_user_index(void);
static int rebuild_user_index(FILE *fp);
static int read_dynamic_line(FILE *fp, char **line_out);
static int append_row(RowArray *row_array, char *row);
static char *duplicate_string(const char *value);
static char *duplicate_range(const char *start, size_t len);
static int write_index_snapshot(void);
static int grow_buffer(char **buffer, size_t *capacity);

static BTreeIndex g_user_index;
static int g_index_loaded = 0;

int append_user_record(const InsertCommand *insert_cmd) {
    /* INSERT 데이터를 CSV 한 줄로 바꿔 파일 끝에 추가한다.
       쓰기 전에 B-tree 인덱스를 준비해 두고, append 후에는 새 id를 인덱스에도 반영한다. */
    FILE *fp;
    int i;
    long row_offset;
    int id;
    int status;

    if (insert_cmd == NULL) {
        return -1;
    }

    if (ensure_user_index_loaded() != 0) {
        return -1;
    }

    fp = fopen(DATA_FILE_PATH, "a+");
    if (fp == NULL) {
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    row_offset = ftell(fp);
    if (row_offset < 0) {
        fclose(fp);
        return -1;
    }

    for (i = 0; i < USER_COLUMN_COUNT; i++) {
        /* 첫 칼럼을 제외하고는 앞에 쉼표를 붙인다. */
        if (append_field_separator(fp, i) != 0) {
            fclose(fp);
            return -1;
        }

        /* id, age는 숫자 컬럼이라 따옴표 없이 저장하고,
           나머지 문자열 컬럼은 큰따옴표로 감싸서 저장한다. */
        if (i == 0 || i == 3) {
            write_csv_field(fp, insert_cmd->values[i], 0);
        } else {
            write_csv_field(fp, insert_cmd->values[i], 1);
        }
    }

    /* 한 행 기록이 끝났으므로 줄바꿈 추가 */
    if (fputc('\n', fp) == EOF) {
        fclose(fp);
        return -1;
    }

    if (fclose(fp) != 0) {
        return -1;
    }

    id = atoi(insert_cmd->values[0]);
    status = btree_insert(&g_user_index, id, row_offset);
    if (status != 0) {
        return -1;
    }

    if (write_index_snapshot() != 0) {
        return -1;
    }

    return 0;
}

int read_all_users(RowArray *row_array) {
    /* CSV 파일 전체를 한 줄씩 읽어 동적 배열에 담는다. */
    FILE *fp;
    char *line = NULL;
    int status;

    if (row_array == NULL) {
        return -1;
    }

    row_array->rows = NULL;
    row_array->count = 0;
    row_array->capacity = 0;

    fp = fopen(DATA_FILE_PATH, "r");
    if (fp == NULL) {
        return -1;
    }

    while ((status = read_dynamic_line(fp, &line)) > 0) {
        if (is_blank_string(line)) {
            free(line);
            line = NULL;
            continue;
        }

        if (append_row(row_array, line) != 0) {
            free(line);
            fclose(fp);
            free_row_array(row_array);
            return -1;
        }

        line = NULL;
    }

    if (status < 0 || ferror(fp)) {
        fclose(fp);
        free_row_array(row_array);
        return -1;
    }

    fclose(fp);
    return 0;
}

int read_user_row_by_id(int id, char **row_out) {
    /* B-tree 인덱스를 통해 id가 가리키는 파일 오프셋을 찾고,
       해당 한 줄만 직접 읽어온다. */
    FILE *fp;
    long row_offset;
    int found;
    int status;

    if (row_out == NULL) {
        return -1;
    }

    *row_out = NULL;

    if (ensure_user_index_loaded() != 0) {
        return -1;
    }

    found = btree_search(&g_user_index, id, &row_offset);
    if (!found) {
        return 1;
    }

    fp = fopen(DATA_FILE_PATH, "r");
    if (fp == NULL) {
        return -1;
    }

    if (fseek(fp, row_offset, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }

    status = read_dynamic_line(fp, row_out);
    fclose(fp);

    if (status <= 0) {
        free(*row_out);
        *row_out = NULL;
        return -1;
    }

    return 0;
}

int user_id_exists(int id) {
    if (ensure_user_index_loaded() != 0) {
        return -1;
    }

    return btree_search(&g_user_index, id, NULL);
}

int split_csv_row(const char *row, char *values[USER_COLUMN_COUNT]) {
    /* CSV 한 줄을 6개 칼럼으로 나눈다.
       큰따옴표 안의 쉼표는 데이터로 취급해야 하므로
       in_quotes 상태를 같이 관리한다. */
    const char *cursor;
    int column = 0;
    int in_quotes = 0;
    char *field;
    size_t field_cap = INITIAL_BUFFER_CAPACITY;
    size_t field_len = 0;

    if (row == NULL || values == NULL) {
        return -1;
    }

    for (column = 0; column < USER_COLUMN_COUNT; column++) {
        values[column] = NULL;
    }

    field = (char *) malloc(field_cap);
    if (field == NULL) {
        return -1;
    }

    cursor = row;
    column = 0;

    while (*cursor != '\0' && column < USER_COLUMN_COUNT) {
        /* 따옴표 밖의 쉼표는 "다음 칼럼으로 이동" 의미다. */
        if (!in_quotes && *cursor == ',') {
            field[field_len] = '\0';
            values[column] = duplicate_string(field);
            if (values[column] == NULL) {
                free(field);
                free_csv_values(values);
                return -1;
            }
            column++;
            field_len = 0;
            cursor++;
            continue;
        }

        /* 큰따옴표는
           - 문자열 시작/끝 표시
           - "" 형태면 실제 따옴표 문자
           를 구분해서 처리해야 한다. */
        if (*cursor == '"') {
            if (in_quotes && cursor[1] == '"') {
                if (field_len + 2 > field_cap && grow_buffer(&field, &field_cap) != 0) {
                    free(field);
                    free_csv_values(values);
                    return -1;
                }
                field[field_len++] = '"';
                cursor += 2;
                continue;
            }

            /* 일반 큰따옴표는 in_quotes 상태를 반전시킨다. */
            in_quotes = !in_quotes;
            cursor++;
            continue;
        }

        /* 일반 문자는 현재 칼럼 버퍼에 그대로 쌓는다. */
        if (field_len + 2 > field_cap && grow_buffer(&field, &field_cap) != 0) {
            free(field);
            free_csv_values(values);
            return -1;
        }

        field[field_len++] = *cursor;
        cursor++;
    }

    /* 문자열 따옴표가 안 닫힌 상태로 끝났으면 잘못된 CSV */
    if (in_quotes) {
        free(field);
        free_csv_values(values);
        return -1;
    }

    field[field_len] = '\0';
    if (column >= USER_COLUMN_COUNT) {
        free(field);
        free_csv_values(values);
        return -1;
    }

    /* 마지막 칼럼도 문자열 종료 표시 */
    values[column] = duplicate_string(field);
    free(field);
    if (values[column] == NULL) {
        free_csv_values(values);
        return -1;
    }
    column++;

    /* users 행은 정확히 6개 칼럼이어야 한다. */
    if (column != USER_COLUMN_COUNT) {
        free_csv_values(values);
        return -1;
    }

    return 0;
}

int row_matches_condition(char *const values[USER_COLUMN_COUNT], const Condition *condition) {
    /* 칼럼 이름을 보고 실제 CSV 배열 위치와 비교한다. */
    if (values == NULL || condition == NULL || !condition->has_condition || condition->value == NULL) {
        return 0;
    }

    if (strcmp(condition->column, "id") == 0) {
        return strcmp(values[0], condition->value) == 0;
    }

    if (strcmp(condition->column, "username") == 0) {
        return strcmp(values[1], condition->value) == 0;
    }

    if (strcmp(condition->column, "name") == 0) {
        return strcmp(values[2], condition->value) == 0;
    }

    if (strcmp(condition->column, "age") == 0) {
        return strcmp(values[3], condition->value) == 0;
    }

    if (strcmp(condition->column, "phone") == 0) {
        return strcmp(values[4], condition->value) == 0;
    }

    if (strcmp(condition->column, "email") == 0) {
        return strcmp(values[5], condition->value) == 0;
    }

    return 0;
}

void write_csv_field(FILE *fp, const char *value, int quote) {
    /* 문자열 안에 " 가 있으면 CSV 규칙에 맞게 "" 로 두 번 써야 한다. */
    const char *cursor;

    if (fp == NULL || value == NULL) {
        return;
    }

    /* 숫자 컬럼은 따옴표 없이 그대로 쓴다. */
    if (!quote) {
        fputs(value, fp);
        return;
    }

    /* 문자열 컬럼은 큰따옴표로 감싸서 CSV 규칙에 맞게 저장한다. */
    fputc('"', fp);
    cursor = value;
    while (*cursor != '\0') {
        /* 실제 따옴표 문자는 "" 로 두 번 써서 저장해야 한다. */
        if (*cursor == '"') {
            fputc('"', fp);
        }
        fputc(*cursor, fp);
        cursor++;
    }
    fputc('"', fp);
}

void free_row_array(RowArray *row_array) {
    int i;

    if (row_array == NULL) {
        return;
    }

    for (i = 0; i < row_array->count; i++) {
        free(row_array->rows[i]);
    }
    free(row_array->rows);
    row_array->rows = NULL;
    row_array->count = 0;
    row_array->capacity = 0;
}

void free_csv_values(char *values[USER_COLUMN_COUNT]) {
    int i;

    if (values == NULL) {
        return;
    }

    for (i = 0; i < USER_COLUMN_COUNT; i++) {
        free(values[i]);
        values[i] = NULL;
    }
}

static int append_field_separator(FILE *fp, int index) {
    /* 첫 칼럼 앞에는 쉼표가 없고, 그 다음 칼럼부터 쉼표를 넣는다. */
    if (index > 0 && fputc(',', fp) == EOF) {
        return -1;
    }
    return 0;
}

static int ensure_user_index_loaded(void) {
    FILE *fp;

    if (g_index_loaded) {
        return 0;
    }

    if (reset_user_index() != 0) {
        return -1;
    }

    fp = fopen(DATA_FILE_PATH, "r");
    if (fp == NULL) {
        g_index_loaded = 1;
        return 0;
    }

    if (rebuild_user_index(fp) != 0) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    g_index_loaded = 1;
    return 0;
}

static int reset_user_index(void) {
    btree_free(&g_user_index);
    btree_init(&g_user_index);
    return 0;
}

static int rebuild_user_index(FILE *fp) {
    char *line = NULL;
    char *values[USER_COLUMN_COUNT];
    long row_offset;
    int status;
    int id;

    while (1) {
        row_offset = ftell(fp);
        if (row_offset < 0) {
            return -1;
        }

        status = read_dynamic_line(fp, &line);
        if (status == 0) {
            return 0;
        }
        if (status < 0) {
            return -1;
        }

        if (is_blank_string(line)) {
            free(line);
            line = NULL;
            continue;
        }

        if (split_csv_row(line, values) != 0) {
            free(line);
            return -1;
        }

        id = atoi(values[0]);
        free_csv_values(values);
        free(line);
        line = NULL;

        status = btree_insert(&g_user_index, id, row_offset);
        if (status < 0) {
            return -1;
        }
    }
}

static int read_dynamic_line(FILE *fp, char **line_out) {
    int ch;
    char *buffer;
    size_t capacity = INITIAL_BUFFER_CAPACITY;
    size_t length = 0;

    if (fp == NULL || line_out == NULL) {
        return -1;
    }

    *line_out = NULL;
    buffer = (char *) malloc(capacity);
    if (buffer == NULL) {
        return -1;
    }

    while ((ch = fgetc(fp)) != EOF) {
        if (length + 2 > capacity && grow_buffer(&buffer, &capacity) != 0) {
            free(buffer);
            return -1;
        }

        if (ch == '\n') {
            break;
        }

        if (ch != '\r') {
            buffer[length++] = (char) ch;
        }
    }

    if (ch == EOF && length == 0) {
        free(buffer);
        return 0;
    }

    buffer[length] = '\0';
    *line_out = buffer;
    return 1;
}

static int append_row(RowArray *row_array, char *row) {
    char **new_rows;
    int new_capacity;

    if (row_array == NULL || row == NULL) {
        return -1;
    }

    if (row_array->count == row_array->capacity) {
        new_capacity = (row_array->capacity == 0) ? 8 : row_array->capacity * 2;
        new_rows = (char **) realloc(row_array->rows, sizeof(char *) * (size_t) new_capacity);
        if (new_rows == NULL) {
            return -1;
        }
        row_array->rows = new_rows;
        row_array->capacity = new_capacity;
    }

    row_array->rows[row_array->count++] = row;
    return 0;
}

static char *duplicate_string(const char *value) {
    size_t len;

    if (value == NULL) {
        return NULL;
    }

    len = strlen(value);
    return duplicate_range(value, len);
}

static char *duplicate_range(const char *start, size_t len) {
    char *copy;

    copy = (char *) malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, start, len);
    copy[len] = '\0';
    return copy;
}

static int write_index_snapshot(void) {
    FILE *fp;

    /* 이번 구현의 B-tree는 메모리 인덱스가 핵심이다.
       users.idx는 "인덱스 사용 중"임을 보여주는 설명용 파일로 유지한다. */
    fp = fopen(INDEX_FILE_PATH, "w");
    if (fp == NULL) {
        return -1;
    }

    fputs("MiniSQL users.id B-tree index is maintained in memory.\n", fp);
    fputs("The index is rebuilt from data/users.csv when the program starts.\n", fp);
    fputs("New INSERT operations update the in-memory index immediately.\n", fp);

    if (fclose(fp) != 0) {
        return -1;
    }

    return 0;
}

static int grow_buffer(char **buffer, size_t *capacity) {
    char *new_buffer;

    if (buffer == NULL || *buffer == NULL || capacity == NULL) {
        return -1;
    }

    *capacity *= 2;
    new_buffer = (char *) realloc(*buffer, *capacity);
    if (new_buffer == NULL) {
        return -1;
    }

    *buffer = new_buffer;
    return 0;
}
