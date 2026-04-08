#include "executor.h"

#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "printer.h"
#include "storage.h"
#include "utils.h"

static int is_supported_table(const char *table);
static ExecStatus validate_insert_command(const InsertCommand *insert_cmd);
static ExecStatus validate_select_command(const SelectCommand *select_cmd);

ExecStatus execute_command(const Command *command) {
    if (command == NULL) {
        return EXEC_UNSUPPORTED_WHERE_CONDITION;
    }

    switch (command->type) {
        case CMD_INSERT:
            return execute_insert(&command->insert_cmd);
        case CMD_SELECT:
            return execute_select(&command->select_cmd);
        case CMD_EXIT:
            return EXEC_OK;
        case CMD_INVALID:
        default:
            return EXEC_UNSUPPORTED_WHERE_CONDITION;
    }
}

ExecStatus execute_insert(const InsertCommand *insert_cmd) {
    ExecStatus status;
    int write_status;

    status = validate_insert_command(insert_cmd);
    if (status != EXEC_OK) {
        return status;
    }

    write_status = append_user_record(insert_cmd);
    if (write_status != 0) {
        return EXEC_WRITE_FAILED;
    }

    print_message("Inserted 1 row");
    return EXEC_OK;
}

ExecStatus execute_select(const SelectCommand *select_cmd) {
    char rows[MAX_INPUT_LEN][MAX_INPUT_LEN];
    int row_count = 0;
    int read_status;
    ExecStatus status;
    int i;
    int matched_count = 0;
    char values[USER_COLUMN_COUNT][128];
    FILE *fp;
    int header_printed = 0;

    status = validate_select_command(select_cmd);
    if (status != EXEC_OK) {
        return status;
    }

    fp = fopen(DATA_FILE_PATH, "r");
    if (fp == NULL) {
        return EXEC_DATA_FILE_NOT_FOUND;
    }
    fclose(fp);

    read_status = read_all_users(rows, &row_count);
    if (read_status != 0) {
        return EXEC_READ_FAILED;
    }

    if (row_count <= 0) {
        return EXEC_NO_ROWS_FOUND;
    }

    for (i = 0; i < row_count; i++) {
        if (split_csv_row(rows[i], values) != 0) {
            continue;
        }

        if (!select_cmd->condition.has_condition || row_matches_condition(values, &select_cmd->condition)) {
            if (!header_printed) {
                print_select_header();
                header_printed = 1;
            }
            print_user_row(values);
            matched_count++;
        }
    }

    if (matched_count == 0) {
        return EXEC_NO_ROWS_FOUND;
    }

    print_rows_selected(matched_count);
    return EXEC_OK;
}

static int is_supported_table(const char *table) {
    return table != NULL && strcmp(table, "users") == 0;
}

static ExecStatus validate_insert_command(const InsertCommand *insert_cmd) {
    if (insert_cmd == NULL) {
        return EXEC_INSERT_VALUE_COUNT_MISMATCH;
    }

    if (!is_supported_table(insert_cmd->table)) {
        return EXEC_UNSUPPORTED_TABLE;
    }

    if (insert_cmd->value_count != USER_COLUMN_COUNT) {
        return EXEC_INSERT_VALUE_COUNT_MISMATCH;
    }

    if (!is_integer_string(insert_cmd->values[0])) {
        return EXEC_INVALID_ID;
    }

    if (!is_integer_string(insert_cmd->values[3])) {
        return EXEC_INVALID_AGE;
    }

    return EXEC_OK;
}

static ExecStatus validate_select_command(const SelectCommand *select_cmd) {
    if (select_cmd == NULL) {
        return EXEC_UNSUPPORTED_SELECT_COLUMNS;
    }

    if (!is_supported_table(select_cmd->table)) {
        return EXEC_UNSUPPORTED_TABLE;
    }

    if (select_cmd->condition.has_condition) {
        if (strcmp(select_cmd->condition.column, "id") != 0) {
            return EXEC_UNSUPPORTED_WHERE_CONDITION;
        }

        if (!is_integer_string(select_cmd->condition.value)) {
            return EXEC_INVALID_ID;
        }
    }

    return EXEC_OK;
}
