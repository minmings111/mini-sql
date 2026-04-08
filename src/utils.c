#include "utils.h"

#include <ctype.h>
#include <string.h>

void trim_newline(char *str) {
    size_t len;

    if (str == NULL) {
        return;
    }

    len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

void trim_spaces(char *str) {
    char *start;
    size_t len;

    if (str == NULL) {
        return;
    }

    start = str;
    while (*start != '\0' && isspace((unsigned char) *start)) {
        start++;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    len = strlen(str);
    while (len > 0 && isspace((unsigned char) str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

void strip_matching_quotes(char *str) {
    size_t len;

    if (str == NULL) {
        return;
    }

    len = strlen(str);
    if (len >= 2 && ((str[0] == '\'' && str[len - 1] == '\'') || (str[0] == '"' && str[len - 1] == '"'))) {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}

int starts_with_ignore_case(const char *str, const char *prefix) {
    size_t i;

    if (str == NULL || prefix == NULL) {
        return 0;
    }

    for (i = 0; prefix[i] != '\0'; i++) {
        if (toupper((unsigned char) str[i]) != toupper((unsigned char) prefix[i])) {
            return 0;
        }
    }

    return 1;
}

int is_quoted_string(const char *str) {
    size_t len;

    if (str == NULL) {
        return 0;
    }

    len = strlen(str);
    if (len < 2) {
        return 0;
    }

    return (str[0] == '\'' && str[len - 1] == '\'')
        || (str[0] == '"' && str[len - 1] == '"');
}

int is_blank_string(const char *str) {
    if (str == NULL) {
        return 1;
    }

    while (*str != '\0') {
        if (!isspace((unsigned char) *str)) {
            return 0;
        }
        str++;
    }

    return 1;
}

int is_integer_string(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }

    if (*str == '+' || *str == '-') {
        str++;
    }

    if (*str == '\0') {
        return 0;
    }

    while (*str != '\0') {
        if (!isdigit((unsigned char) *str)) {
            return 0;
        }
        str++;
    }

    return 1;
}

size_t copy_trimmed(char *dest, size_t dest_size, const char *src) {
    size_t start = 0;
    size_t end;
    size_t len;

    if (dest == NULL || dest_size == 0) {
        return 0;
    }

    if (src == NULL) {
        dest[0] = '\0';
        return 0;
    }

    end = strlen(src);
    while (src[start] != '\0' && isspace((unsigned char) src[start])) {
        start++;
    }
    while (end > start && isspace((unsigned char) src[end - 1])) {
        end--;
    }

    len = end - start;
    if (len >= dest_size) {
        len = dest_size - 1;
    }

    memcpy(dest, src + start, len);
    dest[len] = '\0';
    return len;
}
