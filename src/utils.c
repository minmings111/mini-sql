#include "utils.h"

#include <ctype.h>
#include <string.h>

void trim_newline(char *str) {
    /* fgets는 줄 끝 개행을 같이 읽기 때문에 먼저 제거해 준다. */
    size_t len;

    if (str == NULL) {
        return;
    }

    len = strlen(str);
    /* 줄 끝에 \n, \r 가 남아 있는 동안 반복해서 제거한다. */
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

void trim_spaces(char *str) {
    /* 문자열 앞뒤 공백을 제거하는 helper 함수다. */
    char *start;
    size_t len;

    if (str == NULL) {
        return;
    }

    start = str;
    /* 앞 공백이 끝나는 위치까지 이동 */
    while (*start != '\0' && isspace((unsigned char) *start)) {
        start++;
    }

    /* 앞쪽 공백이 있었다면 문자열 전체를 앞으로 당긴다. */
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    len = strlen(str);
    /* 이번에는 뒤 공백을 끝에서부터 지운다. */
    while (len > 0 && isspace((unsigned char) str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

void strip_matching_quotes(char *str) {
    /* 양끝이 같은 따옴표라면 바깥 따옴표만 제거한다. */
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
    /* 대소문자를 무시한 startsWith 비교 */
    size_t i;

    if (str == NULL || prefix == NULL) {
        return 0;
    }

    /* prefix 끝까지 한 글자씩 비교 */
    for (i = 0; prefix[i] != '\0'; i++) {
        if (toupper((unsigned char) str[i]) != toupper((unsigned char) prefix[i])) {
            return 0;
        }
    }

    return 1;
}

int is_quoted_string(const char *str) {
    /* "abc" 또는 'abc'처럼 양끝이 따옴표로 닫혔는지 검사 */
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
    /* 공백만 있거나 빈 문자열이면 true */
    if (str == NULL) {
        return 1;
    }

    /* 공백이 아닌 문자를 만나면 빈 문자열이 아니다. */
    while (*str != '\0') {
        if (!isspace((unsigned char) *str)) {
            return 0;
        }
        str++;
    }

    return 1;
}

int is_integer_string(const char *str) {
    /* id, age처럼 숫자여야 하는 값 검사용 helper */
    if (str == NULL || *str == '\0') {
        return 0;
    }

    /* 맨 앞의 +, - 부호는 한 번 허용 */
    if (*str == '+' || *str == '-') {
        str++;
    }

    if (*str == '\0') {
        return 0;
    }

    /* 남은 모든 문자가 숫자인지 검사 */
    while (*str != '\0') {
        if (!isdigit((unsigned char) *str)) {
            return 0;
        }
        str++;
    }

    return 1;
}

size_t copy_trimmed(char *dest, size_t dest_size, const char *src) {
    /* src의 앞뒤 공백을 제거한 뒤 dest에 복사한다. */
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

    /* 앞 공백 시작점과 뒤 공백 끝점을 찾는다. */
    end = strlen(src);
    while (src[start] != '\0' && isspace((unsigned char) src[start])) {
        start++;
    }
    while (end > start && isspace((unsigned char) src[end - 1])) {
        end--;
    }

    /* dest 버퍼보다 길면 잘라서 복사 */
    len = end - start;
    if (len >= dest_size) {
        len = dest_size - 1;
    }

    memcpy(dest, src + start, len);
    dest[len] = '\0';
    return len;
}
