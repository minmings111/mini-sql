#include "executor.h"

int execute_command(const Command *command) {
    (void) command;
    return 0;
}

int execute_insert(const InsertCommand *insert_cmd) {
    (void) insert_cmd;
    return 0;
}

int execute_select(const SelectCommand *select_cmd) {
    (void) select_cmd;
    return 0;
}
