#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "types.h"

int execute_command(const Command *command);
int execute_insert(const InsertCommand *insert_cmd);
int execute_select(const SelectCommand *select_cmd);

#endif
