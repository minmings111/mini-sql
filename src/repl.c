#include "repl.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "executor.h"
#include "parser.h"
#include "utils.h"

static void print_parse_error(ParseStatus status);
static void print_exec_error(ExecStatus status);
static int has_complete_statement(const char *buffer);
static int append_input_line(char **buffer, size_t *capacity, const char *line);
static int read_input_line(char **line_out);

int run_repl(void) {
    /* line: 사용자가 방금 입력한 한 줄
       buffer: 여러 줄 입력을 하나의 MiniSQL 문장으로 모아두는 동적 공간 */
    char *line = NULL;
    char *buffer = NULL;
    size_t buffer_capacity = 0;
    int collecting = 0;
    int read_status;

    buffer = (char *) malloc(INITIAL_BUFFER_CAPACITY);
    if (buffer == NULL) {
        puts("Error: out of memory");
        return 1;
    }
    buffer[0] = '\0';
    buffer_capacity = INITIAL_BUFFER_CAPACITY;

    while (1) {
        /* 현재 입력 상태에 따라 적절한 프롬프트를 보여준다. */
        fputs(collecting ? CONTINUATION_PROMPT : PROMPT_TEXT, stdout);
        fflush(stdout);

        read_status = read_input_line(&line);
        if (read_status == 0) {
            /* Ctrl + D 같은 EOF가 들어오면 조용히 종료한다. */
            fputc('\n', stdout);
            free(buffer);
            return 0;
        }
        if (read_status < 0) {
            puts("Error: out of memory");
            free(buffer);
            return 1;
        }

        /* 입력 줄 끝의 개행과 앞뒤 공백을 정리한다. */
        trim_newline(line);
        trim_spaces(line);

        /* 빈 줄은 명령으로 보지 않고 다시 입력받는다. */
        if (is_blank_string(line)) {
            free(line);
            line = NULL;
            continue;
        }

        /* 종료 명령은 파서까지 보내지 않고 REPL 단계에서 바로 처리한다. */
        if (is_exit_command(line)) {
            free(line);
            free(buffer);
            return 0;
        }

        /* 여러 줄 입력을 하나의 버퍼에 이어 붙인다. */
        if (append_input_line(&buffer, &buffer_capacity, line) != 0) {
            puts("Error: out of memory");
            free(line);
            free(buffer);
            return 1;
        }

        free(line);
        line = NULL;

        /* 세미콜론이 아직 없으면 문장이 덜 끝난 것으로 보고 다음 줄을 기다린다. */
        if (!has_complete_statement(buffer)) {
            collecting = 1;
            continue;
        }

        /* 문장이 완성되면 파싱과 실행 단계로 넘긴다. */
        process_input_line(buffer);

        /* 한 문장을 처리했으니 누적 버퍼를 비운다. */
        buffer[0] = '\0';
        collecting = 0;
    }
}

int process_input_line(const char *line) {
    /* parser가 문자열을 구조체로 바꾸고,
       executor가 그 구조체를 실제 동작으로 바꾼다. */
    Command command;
    ParseStatus parse_status;
    ExecStatus exec_status;

    init_command(&command);

    /* 먼저 문자열을 구조체 형태로 해석한다. */
    parse_status = parse_command(line, &command);
    if (parse_status != PARSE_OK) {
        print_parse_error(parse_status);
        free_command(&command);
        return -1;
    }

    /* 문법이 맞다면 이제 실제 저장/조회 동작을 수행한다. */
    exec_status = execute_command(&command);
    if (exec_status != EXEC_OK && exec_status != EXEC_NO_ROWS_FOUND) {
        print_exec_error(exec_status);
        free_command(&command);
        return -1;
    }

    /* SELECT는 문법상 맞더라도 결과가 0건일 수 있으므로
       오류가 아니라 별도 상태로 처리한다. */
    if (exec_status == EXEC_NO_ROWS_FOUND) {
        puts("No rows found");
    }

    free_command(&command);
    return 0;
}

int is_exit_command(const char *line) {
    return (starts_with_ignore_case(line, "exit") && line[4] == '\0')
        || (starts_with_ignore_case(line, "quit") && line[4] == '\0');
}

static void print_parse_error(ParseStatus status) {
    /* parser 내부 상태 코드를 사용자용 문장으로 바꾼다. */
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
        case PARSE_OUT_OF_MEMORY:
            puts("Error: out of memory");
            break;
        case PARSE_OK:
        default:
            break;
    }
}

static void print_exec_error(ExecStatus status) {
    /* executor 내부 상태 코드도 같은 방식으로 출력한다. */
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
            puts("Error: INSERT expects 5 values");
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
        case EXEC_MEMORY_ERROR:
            puts("Error: out of memory");
            break;
        case EXEC_OK:
        case EXEC_NO_ROWS_FOUND:
        default:
            break;
    }
}

static int has_complete_statement(const char *buffer) {
    /* 따옴표 바깥에서 세미콜론이 나왔을 때만
       "문장이 끝났다"라고 판단한다. */
    int in_single = 0;
    int in_double = 0;
    const char *cursor;

    if (buffer == NULL) {
        return 0;
    }

    cursor = buffer;
    while (*cursor != '\0') {
        /* 작은따옴표 안으로 들어가면
           다음 작은따옴표가 나올 때까지는 문자열 내부라고 본다. */
        if (*cursor == '\'' && !in_double) {
            in_single = !in_single;
        /* 큰따옴표도 같은 원리로 추적한다. */
        } else if (*cursor == '"' && !in_single) {
            in_double = !in_double;
        /* 따옴표 바깥의 세미콜론만 문장 종료로 인정한다. */
        } else if (*cursor == ';' && !in_single && !in_double) {
            cursor++;
            /* 세미콜론 뒤에는 공백만 남아 있어야 한다. */
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

static int append_input_line(char **buffer, size_t *capacity, const char *line) {
    size_t buffer_len;
    size_t line_len;
    size_t needed;
    char *new_buffer;

    if (buffer == NULL || *buffer == NULL || capacity == NULL || line == NULL) {
        return -1;
    }

    buffer_len = strlen(*buffer);
    line_len = strlen(line);
    needed = buffer_len + line_len + 1;
    if (buffer_len > 0) {
        needed++;
    }

    if (needed > *capacity) {
        while (needed > *capacity) {
            *capacity *= 2;
        }

        new_buffer = (char *) realloc(*buffer, *capacity);
        if (new_buffer == NULL) {
            return -1;
        }
        *buffer = new_buffer;
    }

    /* 이미 앞줄이 있으면 공백 하나를 넣어서 자연스럽게 이어 붙인다. */
    if (buffer_len > 0) {
        (*buffer)[buffer_len++] = ' ';
        (*buffer)[buffer_len] = '\0';
    }

    /* line 전체를 buffer 뒤에 그대로 이어 붙인다. */
    memcpy(*buffer + buffer_len, line, line_len + 1);
    return 0;
}

static int read_input_line(char **line_out) {
    int ch;
    char *buffer;
    size_t capacity = INITIAL_BUFFER_CAPACITY;
    size_t length = 0;
    char *new_buffer;

    if (line_out == NULL) {
        return -1;
    }

    *line_out = NULL;
    buffer = (char *) malloc(capacity);
    if (buffer == NULL) {
        return -1;
    }

    while ((ch = fgetc(stdin)) != EOF) {
        if (length + 2 > capacity) {
            capacity *= 2;
            new_buffer = (char *) realloc(buffer, capacity);
            if (new_buffer == NULL) {
                free(buffer);
                return -1;
            }
            buffer = new_buffer;
        }

        buffer[length++] = (char) ch;
        if (ch == '\n') {
            break;
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
