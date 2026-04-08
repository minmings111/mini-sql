#include "storage.h"

int append_user_record(const InsertCommand *insert_cmd) {
    (void) insert_cmd;
    return 0;
}

int read_all_users(char rows[][MAX_INPUT_LEN], int *row_count) {
    (void) rows;
    (void) row_count;
    return 0;
}

int split_csv_row(const char *row, char values[USER_COLUMN_COUNT][128]) {
    (void) row;
    (void) values;
    return 0;
}

int row_matches_condition(const char values[USER_COLUMN_COUNT][128], const Condition *condition) {
    (void) values;
    (void) condition;
    return 0;
}

void write_csv_field(FILE *fp, const char *value, int quote) {
    (void) fp;
    (void) value;
    (void) quote;
}
