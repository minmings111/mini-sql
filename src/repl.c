#include "repl.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "executor.h"
#include "parser.h"
#include "utils.h"

static void print_parse_error(ParseStatus status);
static void print_exec_error(ExecStatus status);
static int has_complete_statement(const char *buffer);
static int append_input_line(char *buffer, size_t buffer_size, const char *line);

int run_repl(void) {
    char line[MAX_INPUT_LEN];
    char buffer[MAX_INPUT_LEN];
    int collecting = 0;

    buffer[0] = '\0';

    while (1) {
        fputs(collecting ? CONTINUATION_PROMPT : PROMPT_TEXT, stdout);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            fputc('\n', stdout);
            return 0;
        }

        trim_newline(line);
        trim_spaces(line);

        if (is_blank_string(line)) {
            continue;
        }

        if (is_exit_command(line)) {
            return 0;
        }

        if (append_input_line(buffer, sizeof(buffer), line) != 0) {
            puts("Error: input too long");
            buffer[0] = '\0';
            collecting = 0;
            continue;
        }

        if (!has_complete_statement(buffer)) {
            collecting = 1;
            continue;
        }

        process_input_line(buffer);
        buffer[0] = '\0';
        collecting = 0;
    }
}

int process_input_line(const char *line) {
    Command command;
    ParseStatus parse_status;
    ExecStatus exec_status;

    parse_status = parse_command(line, &command);
    if (parse_status != PARSE_OK) {
        print_parse_error(parse_status);
        return -1;
    }

    exec_status = execute_command(&command);
    if (exec_status != EXEC_OK && exec_status != EXEC_NO_ROWS_FOUND) {
        print_exec_error(exec_status);
        return -1;
    }

    if (exec_status == EXEC_NO_ROWS_FOUND) {
        puts("No rows found");
    }

    return 0;
}

int is_exit_command(const char *line) {
    return (starts_with_ignore_case(line, "exit") && line[4] == '\0')
        || (starts_with_ignore_case(line, "quit") && line[4] == '\0');
}

static void print_parse_error(ParseStatus status) {
    switch (status) {
        case PARSE_MISSING_SEMICOLON:
            puts("Error: missing semicolon");
            break;
        case PARSE_UNSUPPORTED_COMMAND:
            puts("Error: unsupported command");
            break;
        case PARSE_INVALID_INSERT:
            puts("Error: invalid INSERT syntax");
            break;
        case PARSE_INVALID_SELECT:
            puts("Error: invalid SELECT syntax");
            break;
        case PARSE_INVALID_WHERE:
            puts("Error: invalid WHERE syntax");
            break;
        case PARSE_UNTERMINATED_STRING:
            puts("Error: unterminated string literal");
            break;
        case PARSE_UNSUPPORTED_QUOTED_FORMAT:
            puts("Error: unsupported quoted string format");
            break;
        case PARSE_OK:
        default:
            break;
    }
}

static void print_exec_error(ExecStatus status) {
    switch (status) {
        case EXEC_UNSUPPORTED_TABLE:
            puts("Error: unsupported table");
            break;
        case EXEC_UNSUPPORTED_SELECT_COLUMNS:
            puts("Error: unsupported SELECT columns");
            break;
        case EXEC_UNSUPPORTED_WHERE_CONDITION:
            puts("Error: unsupported WHERE condition");
            break;
        case EXEC_INSERT_VALUE_COUNT_MISMATCH:
            puts("Error: INSERT expects 6 values");
            break;
        case EXEC_INVALID_ID:
            puts("Error: invalid numeric value for id");
            break;
        case EXEC_INVALID_AGE:
            puts("Error: invalid numeric value for age");
            break;
        case EXEC_DATA_FILE_NOT_FOUND:
            puts("Error: data file not found");
            break;
        case EXEC_READ_FAILED:
            puts("Error: failed to read data file");
            break;
        case EXEC_WRITE_FAILED:
            puts("Error: failed to write data file");
            break;
        case EXEC_OK:
        case EXEC_NO_ROWS_FOUND:
        default:
            break;
    }
}

static int has_complete_statement(const char *buffer) {
    int in_single = 0;
    int in_double = 0;
    const char *cursor;

    if (buffer == NULL) {
        return 0;
    }

    cursor = buffer;
    while (*cursor != '\0') {
        if (*cursor == '\'' && !in_double) {
            in_single = !in_single;
        } else if (*cursor == '"' && !in_single) {
            in_double = !in_double;
        } else if (*cursor == ';' && !in_single && !in_double) {
            cursor++;
            while (*cursor != '\0') {
                if (!isspace((unsigned char) *cursor)) {
                    return 1;
                }
                cursor++;
            }
            return 1;
        }
        cursor++;
    }

    return 0;
}

static int append_input_line(char *buffer, size_t buffer_size, const char *line) {
    size_t buffer_len;
    size_t line_len;

    if (buffer == NULL || line == NULL || buffer_size == 0) {
        return -1;
    }

    buffer_len = strlen(buffer);
    line_len = strlen(line);

    if (buffer_len > 0) {
        if (buffer_len + 1 >= buffer_size) {
            return -1;
        }
        buffer[buffer_len++] = ' ';
        buffer[buffer_len] = '\0';
    }

    if (buffer_len + line_len >= buffer_size) {
        return -1;
    }

    memcpy(buffer + buffer_len, line, line_len + 1);
    return 0;
}
