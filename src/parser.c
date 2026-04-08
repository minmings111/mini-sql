#include "parser.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

#include "constants.h"
#include "utils.h"

static const char *skip_spaces(const char *s);
static int match_keyword(const char **input, const char *keyword);
static int parse_identifier_token(const char **input, char *dest, size_t dest_size);
static ParseStatus parse_value_token(const char **input, char *dest, size_t dest_size, int *was_quoted);
static ParseStatus parse_values_list(const char **input, InsertCommand *insert_cmd);

ParseStatus parse_command(const char *input, Command *command) {
    const char *cursor;

    if (input == NULL || command == NULL) {
        return PARSE_UNSUPPORTED_COMMAND;
    }

    init_command(command);
    cursor = skip_spaces(input);

    if (starts_with_ignore_case(cursor, "insert")) {
        return parse_insert(cursor, command);
    }

    if (starts_with_ignore_case(cursor, "select")) {
        return parse_select(cursor, command);
    }

    return PARSE_UNSUPPORTED_COMMAND;
}

ParseStatus parse_insert(const char *input, Command *command) {
    const char *cursor = input;
    ParseStatus status;

    if (!match_keyword(&cursor, "INSERT")) {
        return PARSE_INVALID_INSERT;
    }

    if (!match_keyword(&cursor, "INTO")) {
        return PARSE_INVALID_INSERT;
    }

    if (!parse_identifier_token(&cursor, command->insert_cmd.table, sizeof(command->insert_cmd.table))) {
        return PARSE_INVALID_INSERT;
    }

    if (!match_keyword(&cursor, "VALUES")) {
        return PARSE_INVALID_INSERT;
    }

    cursor = skip_spaces(cursor);
    if (*cursor != '(') {
        return PARSE_INVALID_INSERT;
    }
    cursor++;

    status = parse_values_list(&cursor, &command->insert_cmd);
    if (status != PARSE_OK) {
        return status;
    }

    cursor = skip_spaces(cursor);
    if (*cursor != ')') {
        return PARSE_INVALID_INSERT;
    }
    cursor++;

    cursor = skip_spaces(cursor);
    if (*cursor != ';') {
        return PARSE_MISSING_SEMICOLON;
    }
    cursor++;

    if (!is_blank_string(cursor)) {
        return PARSE_INVALID_INSERT;
    }

    command->type = CMD_INSERT;
    return PARSE_OK;
}

ParseStatus parse_select(const char *input, Command *command) {
    const char *cursor = input;
    ParseStatus status;

    if (!match_keyword(&cursor, "SELECT")) {
        return PARSE_INVALID_SELECT;
    }

    cursor = skip_spaces(cursor);
    if (*cursor != '*') {
        return PARSE_INVALID_SELECT;
    }
    cursor++;

    if (!match_keyword(&cursor, "FROM")) {
        return PARSE_INVALID_SELECT;
    }

    if (!parse_identifier_token(&cursor, command->select_cmd.table, sizeof(command->select_cmd.table))) {
        return PARSE_INVALID_SELECT;
    }

    cursor = skip_spaces(cursor);
    if (starts_with_ignore_case(cursor, "WHERE")) {
        status = parse_condition(cursor, &command->select_cmd.condition);
        if (status != PARSE_OK) {
            return status;
        }

        while (*cursor != '\0' && *cursor != ';') {
            cursor++;
        }
    } else {
        command->select_cmd.condition.has_condition = 0;
    }

    cursor = skip_spaces(cursor);
    if (*cursor != ';') {
        return PARSE_MISSING_SEMICOLON;
    }
    cursor++;

    if (!is_blank_string(cursor)) {
        return PARSE_INVALID_SELECT;
    }

    command->type = CMD_SELECT;
    return PARSE_OK;
}

ParseStatus parse_condition(const char *input, Condition *condition) {
    const char *cursor = input;
    int was_quoted = 0;
    ParseStatus status;

    if (condition == NULL) {
        return PARSE_INVALID_WHERE;
    }

    if (!match_keyword(&cursor, "WHERE")) {
        return PARSE_INVALID_WHERE;
    }

    if (!parse_identifier_token(&cursor, condition->column, sizeof(condition->column))) {
        return PARSE_INVALID_WHERE;
    }

    cursor = skip_spaces(cursor);
    if (*cursor != '=') {
        return PARSE_INVALID_WHERE;
    }
    cursor++;

    status = parse_value_token(&cursor, condition->value, sizeof(condition->value), &was_quoted);
    if (status != PARSE_OK) {
        return status == PARSE_INVALID_INSERT ? PARSE_INVALID_WHERE : status;
    }

    cursor = skip_spaces(cursor);
    if (*cursor != ';') {
        return PARSE_INVALID_WHERE;
    }

    condition->has_condition = 1;
    return PARSE_OK;
}

void init_command(Command *command) {
    if (command == NULL) {
        return;
    }

    memset(command, 0, sizeof(*command));
    command->type = CMD_INVALID;
}

static const char *skip_spaces(const char *s) {
    while (s != NULL && *s != '\0' && isspace((unsigned char) *s)) {
        s++;
    }
    return s;
}

static int match_keyword(const char **input, const char *keyword) {
    size_t len;
    const char *cursor;
    size_t i;

    if (input == NULL || *input == NULL || keyword == NULL) {
        return 0;
    }

    cursor = skip_spaces(*input);
    len = strlen(keyword);

    for (i = 0; i < len; i++) {
        if (toupper((unsigned char) cursor[i]) != toupper((unsigned char) keyword[i])) {
            return 0;
        }
    }

    if (isalnum((unsigned char) cursor[len]) || cursor[len] == '_') {
        return 0;
    }

    *input = cursor + len;
    return 1;
}

static int parse_identifier_token(const char **input, char *dest, size_t dest_size) {
    const char *cursor;
    size_t len = 0;

    if (input == NULL || *input == NULL || dest == NULL || dest_size == 0) {
        return 0;
    }

    cursor = skip_spaces(*input);
    if (!isalpha((unsigned char) *cursor) && *cursor != '_') {
        return 0;
    }

    while ((isalnum((unsigned char) cursor[len]) || cursor[len] == '_') && len + 1 < dest_size) {
        dest[len] = cursor[len];
        len++;
    }

    if (isalnum((unsigned char) cursor[len]) || cursor[len] == '_') {
        return 0;
    }

    dest[len] = '\0';
    *input = cursor + len;
    return 1;
}

static ParseStatus parse_value_token(const char **input, char *dest, size_t dest_size, int *was_quoted) {
    const char *cursor;
    size_t len = 0;
    char quote = '\0';

    if (input == NULL || *input == NULL || dest == NULL || dest_size == 0) {
        return PARSE_INVALID_INSERT;
    }

    cursor = skip_spaces(*input);
    if (*cursor == '\0') {
        return PARSE_INVALID_INSERT;
    }

    if (*cursor == '\'' || *cursor == '"') {
        quote = *cursor;
        cursor++;
        if (was_quoted != NULL) {
            *was_quoted = 1;
        }

        while (*cursor != '\0' && *cursor != quote) {
            if (*cursor == '\n' || *cursor == '\r') {
                return PARSE_UNTERMINATED_STRING;
            }
            if (len + 1 >= dest_size) {
                return PARSE_UNSUPPORTED_QUOTED_FORMAT;
            }
            dest[len++] = *cursor++;
        }

        if (*cursor != quote) {
            return PARSE_UNTERMINATED_STRING;
        }

        dest[len] = '\0';
        cursor++;
        cursor = skip_spaces(cursor);
        if (*cursor != ',' && *cursor != ')' && *cursor != ';' && *cursor != '\0') {
            return PARSE_UNSUPPORTED_QUOTED_FORMAT;
        }
        *input = cursor;
        return PARSE_OK;
    }

    if (was_quoted != NULL) {
        *was_quoted = 0;
    }

    while (*cursor != '\0' && *cursor != ',' && *cursor != ')' && *cursor != ';' && !isspace((unsigned char) *cursor)) {
        if (len + 1 >= dest_size) {
            return PARSE_INVALID_INSERT;
        }
        dest[len++] = *cursor++;
    }

    if (len == 0) {
        return PARSE_INVALID_INSERT;
    }

    dest[len] = '\0';
    *input = cursor;
    return PARSE_OK;
}

static ParseStatus parse_values_list(const char **input, InsertCommand *insert_cmd) {
    const char *cursor = *input;
    ParseStatus status;
    int index = 0;

    cursor = skip_spaces(cursor);
    if (*cursor == ')') {
        insert_cmd->value_count = 0;
        *input = cursor;
        return PARSE_OK;
    }

    while (*cursor != '\0' && *cursor != ')') {
        if (index >= USER_COLUMN_COUNT) {
            return PARSE_INVALID_INSERT;
        }

        status = parse_value_token(&cursor, insert_cmd->values[index], sizeof(insert_cmd->values[index]), NULL);
        if (status != PARSE_OK) {
            return status;
        }

        index++;
        cursor = skip_spaces(cursor);

        if (*cursor == ')') {
            break;
        }

        if (*cursor != ',') {
            return PARSE_INVALID_INSERT;
        }
        cursor++;
    }

    cursor = skip_spaces(cursor);
    if (*cursor == ',') {
        return PARSE_INVALID_INSERT;
    }

    insert_cmd->value_count = index;
    *input = cursor;
    return PARSE_OK;
}
