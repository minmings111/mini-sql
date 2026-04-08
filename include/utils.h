#ifndef UTILS_H
#define UTILS_H

void trim_newline(char *str);
void trim_spaces(char *str);
void strip_matching_quotes(char *str);
int starts_with_ignore_case(const char *str, const char *prefix);
int is_quoted_string(const char *str);

#endif
