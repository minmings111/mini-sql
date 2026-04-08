#ifndef TYPES_H
#define TYPES_H

#include "constants.h"

typedef enum {
    CMD_INSERT,
    CMD_SELECT,
    CMD_EXIT,
    CMD_INVALID
} CommandType;

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
