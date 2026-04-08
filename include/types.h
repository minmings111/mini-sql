#ifndef TYPES_H
#define TYPES_H

#include "constants.h"

typedef enum {
    CMD_INSERT,
    CMD_SELECT,
    CMD_EXIT,
    CMD_INVALID
} CommandType;

typedef enum {
    PARSE_OK = 0,
    PARSE_MISSING_SEMICOLON,
    PARSE_UNSUPPORTED_COMMAND,
    PARSE_INVALID_INSERT,
    PARSE_INVALID_SELECT,
    PARSE_INVALID_WHERE,
    PARSE_UNTERMINATED_STRING,
    PARSE_UNSUPPORTED_QUOTED_FORMAT
} ParseStatus;

typedef enum {
    EXEC_OK = 0,
    EXEC_UNSUPPORTED_TABLE,
    EXEC_UNSUPPORTED_SELECT_COLUMNS,
    EXEC_UNSUPPORTED_WHERE_CONDITION,
    EXEC_INSERT_VALUE_COUNT_MISMATCH,
    EXEC_INVALID_ID,
    EXEC_INVALID_AGE,
    EXEC_DATA_FILE_NOT_FOUND,
    EXEC_READ_FAILED,
    EXEC_WRITE_FAILED,
    EXEC_NO_ROWS_FOUND
} ExecStatus;

typedef struct {
    char column[32];
    char value[128];
    int has_condition;
} Condition;

typedef struct {
    char table[32];
    char values[USER_COLUMN_COUNT][128];
    int value_count;
} InsertCommand;

typedef struct {
    char table[32];
    Condition condition;
} SelectCommand;

typedef struct {
    CommandType type;
    InsertCommand insert_cmd;
    SelectCommand select_cmd;
} Command;

#endif
