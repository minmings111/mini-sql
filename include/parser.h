#ifndef PARSER_H
#define PARSER_H

#include "types.h"

int parse_command(const char *input, Command *command);
int parse_insert(const char *input, Command *command);
int parse_select(const char *input, Command *command);
int parse_condition(const char *input, Condition *condition);
void init_command(Command *command);

#endif
