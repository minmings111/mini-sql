#include "storage.h"

#include <stdio.h>
#include <string.h>

#include "utils.h"

static int append_field_separator(FILE *fp, int index);

int append_user_record(const InsertCommand *insert_cmd) {
    FILE *fp;
    int i;

    if (insert_cmd == NULL) {
        return -1;
    }

    fp = fopen(DATA_FILE_PATH, "a");
    if (fp == NULL) {
        return -1;
    }

    for (i = 0; i < USER_COLUMN_COUNT; i++) {
        if (append_field_separator(fp, i) != 0) {
            fclose(fp);
            return -1;
        }

        if (i == 0 || i == 3) {
            write_csv_field(fp, insert_cmd->values[i], 0);
        } else {
            write_csv_field(fp, insert_cmd->values[i], 1);
        }
    }

    if (fputc('\n', fp) == EOF) {
        fclose(fp);
        return -1;
    }

    if (fclose(fp) != 0) {
        return -1;
    }

    return 0;
}

int read_all_users(char rows[][MAX_INPUT_LEN], int *row_count) {
    FILE *fp;
    char buffer[MAX_INPUT_LEN];
    int count = 0;

    if (rows == NULL || row_count == NULL) {
        return -1;
    }

    fp = fopen(DATA_FILE_PATH, "r");
    if (fp == NULL) {
        return -1;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        trim_newline(buffer);
        if (is_blank_string(buffer)) {
            continue;
        }

        strncpy(rows[count], buffer, MAX_INPUT_LEN - 1);
        rows[count][MAX_INPUT_LEN - 1] = '\0';
        count++;
    }

    if (ferror(fp)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    *row_count = count;
    return 0;
}

int split_csv_row(const char *row, char values[USER_COLUMN_COUNT][128]) {
    const char *cursor;
    int column = 0;
    int index = 0;
    int in_quotes = 0;

    if (row == NULL || values == NULL) {
        return -1;
    }

    for (column = 0; column < USER_COLUMN_COUNT; column++) {
        values[column][0] = '\0';
    }

    cursor = row;
    column = 0;
    index = 0;

    while (*cursor != '\0' && column < USER_COLUMN_COUNT) {
        if (!in_quotes && *cursor == ',') {
            values[column][index] = '\0';
            column++;
            index = 0;
            cursor++;
            continue;
        }

        if (*cursor == '"') {
            if (in_quotes && cursor[1] == '"') {
                if (index + 1 >= 128) {
                    return -1;
                }
                values[column][index++] = '"';
                cursor += 2;
                continue;
            }

            in_quotes = !in_quotes;
            cursor++;
            continue;
        }

        if (index + 1 >= 128) {
            return -1;
        }

        values[column][index++] = *cursor;
        cursor++;
    }

    if (in_quotes) {
        return -1;
    }

    if (column >= USER_COLUMN_COUNT) {
        return -1;
    }

    values[column][index] = '\0';
    column++;

    if (column != USER_COLUMN_COUNT) {
        return -1;
    }

    return 0;
}

int row_matches_condition(const char values[USER_COLUMN_COUNT][128], const Condition *condition) {
    if (values == NULL || condition == NULL || !condition->has_condition) {
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
    const char *cursor;

    if (fp == NULL || value == NULL) {
        return;
    }

    if (!quote) {
        fputs(value, fp);
        return;
    }

    fputc('"', fp);
    cursor = value;
    while (*cursor != '\0') {
        if (*cursor == '"') {
            fputc('"', fp);
        }
        fputc(*cursor, fp);
        cursor++;
    }
    fputc('"', fp);
}

static int append_field_separator(FILE *fp, int index) {
    if (index > 0 && fputc(',', fp) == EOF) {
        return -1;
    }
    return 0;
}
