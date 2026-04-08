#include "executor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "printer.h"
#include "storage.h"
#include "utils.h"

static int is_supported_table(const char *table);
static int is_supported_where_column(const char *column);
static ExecStatus validate_insert_command(const InsertCommand *insert_cmd);
static ExecStatus validate_select_command(const SelectCommand *select_cmd);

ExecStatus execute_command(const Command *command) {
    /* parser가 만든 Command 구조체를 보고
       어떤 실행 함수를 부를지 결정한다. */
    if (command == NULL) {
        return EXEC_UNSUPPORTED_WHERE_CONDITION;
    }

    switch (command->type) {
        /* INSERT 구조체를 INSERT 실행 함수로 전달 */
        case CMD_INSERT:
            return execute_insert(&command->insert_cmd);
        /* SELECT 구조체를 SELECT 실행 함수로 전달 */
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
    /* INSERT는 검증 -> CSV 파일에 append -> 성공 메시지 출력 순서로 동작한다. */
    ExecStatus status;
    int write_status;

    /* 실제 파일 쓰기 전에 형식과 타입이 맞는지 먼저 확인 */
    status = validate_insert_command(insert_cmd);
    if (status != EXEC_OK) {
        return status;
    }

    /* 검증이 끝난 후에만 CSV 파일 끝에 새 행을 추가한다. */
    write_status = append_user_record(insert_cmd);
    if (write_status != 0) {
        return EXEC_WRITE_FAILED;
    }

    print_message("Inserted 1 row");
    return EXEC_OK;
}

ExecStatus execute_select(const SelectCommand *select_cmd) {
    /* SELECT는 조건이 id이면 B-tree 인덱스로 바로 찾고,
       다른 컬럼 조건이거나 전체 조회면 CSV 전체를 읽어 순회한다. */
    ExecStatus status;
    char *row = NULL;
    char *values[USER_COLUMN_COUNT] = {0};
    int read_status;
    int i;
    int matched_count = 0;
    RowArray row_array;
    FILE *fp;

    /* 현재 프로젝트가 지원하는 SELECT 형태인지 검사 */
    status = validate_select_command(select_cmd);
    if (status != EXEC_OK) {
        return status;
    }

    /* SELECT는 먼저 데이터 파일이 실제로 존재하는지 확인한다. */
    fp = fopen(DATA_FILE_PATH, "r");
    if (fp == NULL) {
        return EXEC_DATA_FILE_NOT_FOUND;
    }
    fclose(fp);

    /* WHERE id = 값 형태는 B-tree 인덱스로 필요한 한 줄만 직접 찾는다. */
    if (select_cmd->condition.has_condition && strcmp(select_cmd->condition.column, "id") == 0) {
        read_status = read_user_row_by_id(atoi(select_cmd->condition.value), &row);
        if (read_status < 0) {
            return EXEC_READ_FAILED;
        }
        if (read_status > 0) {
            return EXEC_NO_ROWS_FOUND;
        }

        if (split_csv_row(row, values) != 0) {
            free(row);
            return EXEC_READ_FAILED;
        }

        print_select_header();
        print_user_row(values);
        print_rows_selected(1);

        free(row);
        free_csv_values(values);
        return EXEC_OK;
    }

    /* 전체 조회이거나 id 이외 컬럼 조건은 파일의 모든 행을 읽어 순회한다. */
    row_array.rows = NULL;
    row_array.count = 0;
    row_array.capacity = 0;

    read_status = read_all_users(&row_array);
    if (read_status != 0) {
        return EXEC_READ_FAILED;
    }

    if (row_array.count == 0) {
        free_row_array(&row_array);
        return EXEC_NO_ROWS_FOUND;
    }

    /* 읽어온 각 행을 하나씩 검사한다. */
    for (i = 0; i < row_array.count; i++) {
        if (split_csv_row(row_array.rows[i], values) != 0) {
            /* 손상된 줄은 무시하고 다음 줄로 넘어간다. */
            free_csv_values(values);
            continue;
        }

        /* WHERE가 있으면 현재 행이 조건과 맞는지 확인한다. */
        if (select_cmd->condition.has_condition && !row_matches_condition(values, &select_cmd->condition)) {
            free_csv_values(values);
            continue;
        }

        if (matched_count == 0) {
            /* 첫 결과가 나올 때만 헤더를 한 번 출력 */
            print_select_header();
        }

        print_user_row(values);
        matched_count++;
        free_csv_values(values);
    }

    free_row_array(&row_array);

    /* 조건에 맞는 행이 없으면 별도 상태를 돌려준다. */
    if (matched_count == 0) {
        return EXEC_NO_ROWS_FOUND;
    }

    /* 최종 몇 행이 선택됐는지 표시 */
    print_rows_selected(matched_count);
    return EXEC_OK;
}

static int is_supported_table(const char *table) {
    return table != NULL && strcmp(table, "users") == 0;
}

static int is_supported_where_column(const char *column) {
    if (column == NULL) {
        return 0;
    }

    return strcmp(column, "id") == 0
        || strcmp(column, "username") == 0
        || strcmp(column, "name") == 0
        || strcmp(column, "age") == 0
        || strcmp(column, "phone") == 0
        || strcmp(column, "email") == 0;
}

static ExecStatus validate_insert_command(const InsertCommand *insert_cmd) {
    /* 1차 구현에서는 users 테이블, 5개 사용자 입력 값,
       그리고 age 숫자 여부를 확인한다.
       id는 storage 단계에서 자동 증가로 생성한다. */
    if (insert_cmd == NULL) {
        return EXEC_INSERT_VALUE_COUNT_MISMATCH;
    }

    /* 현재는 users 테이블 하나만 지원 */
    if (!is_supported_table(insert_cmd->table)) {
        return EXEC_UNSUPPORTED_TABLE;
    }

    /* 사용자는 username, name, age, phone, email의 5개 값만 입력한다. */
    if (insert_cmd->value_count != USER_INPUT_VALUE_COUNT) {
        return EXEC_INSERT_VALUE_COUNT_MISMATCH;
    }

    /* age도 숫자 */
    if (insert_cmd->values[2] == NULL || !is_integer_string(insert_cmd->values[2])) {
        return EXEC_INVALID_AGE;
    }

    return EXEC_OK;
}

static ExecStatus validate_select_command(const SelectCommand *select_cmd) {
    /* 1차 구현에서는 users 테이블의 단일 WHERE column = value 조건을 지원한다. */
    if (select_cmd == NULL) {
        return EXEC_UNSUPPORTED_SELECT_COLUMNS;
    }

    if (!is_supported_table(select_cmd->table)) {
        return EXEC_UNSUPPORTED_TABLE;
    }

    /* WHERE가 있다면 현재 범위 안의 조건인지 검사 */
    if (select_cmd->condition.has_condition) {
        /* users의 고정 컬럼만 WHERE에서 허용한다. */
        if (!is_supported_where_column(select_cmd->condition.column)) {
            return EXEC_UNSUPPORTED_WHERE_CONDITION;
        }

        if (select_cmd->condition.value == NULL) {
            return EXEC_UNSUPPORTED_WHERE_CONDITION;
        }

        /* 숫자 컬럼인 id, age는 비교값도 숫자여야 한다. */
        if (strcmp(select_cmd->condition.column, "id") == 0
            && !is_integer_string(select_cmd->condition.value)) {
            return EXEC_INVALID_ID;
        }
        if (strcmp(select_cmd->condition.column, "age") == 0
            && !is_integer_string(select_cmd->condition.value)) {
            return EXEC_INVALID_AGE;
        }
    }

    return EXEC_OK;
}
