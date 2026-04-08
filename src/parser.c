#include "parser.h"

int parse_command(const char *input, Command *command) {
    (void) input;
    init_command(command);
    return 0;
}

int parse_insert(const char *input, Command *command) {
    (void) input;
    (void) command;
    return 0;
}

int parse_select(const char *input, Command *command) {
    (void) input;
    (void) command;
    return 0;
}

int parse_condition(const char *input, Condition *condition) {
    (void) input;
    (void) condition;
    return 0;
}

void init_command(Command *command) {
    if (command == 0) {
        return;
    }

    command->type = CMD_INVALID;
}
