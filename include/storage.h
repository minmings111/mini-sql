#ifndef STORAGE_H
#define STORAGE_H

#include <stdio.h>

#include "constants.h"
#include "types.h"

int append_user_record(const InsertCommand *insert_cmd);
int read_all_users(char rows[][MAX_INPUT_LEN], int *row_count);
int split_csv_row(const char *row, char values[USER_COLUMN_COUNT][128]);
int row_matches_condition(const char values[USER_COLUMN_COUNT][128], const Condition *condition);
void write_csv_field(FILE *fp, const char *value, int quote);

#endif
