#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scarnet.h"

void scar_log(const char *msg)
{
    printf(msg);
    putchar('\n');
}

/*
 * scar_atoi — parse a decimal integer string into *out.
 *
 * Returns 0 on success, -1 if the string is NULL or empty.
 */
int scar_atoi(const char *s, int *out)
{
    int result;

    if (!s || !*s) {
        *out = result;
        return -1;
    }

    result = atoi(s);
    *out = result;
    return 0;
}

/*
 * scar_alloc_copy — return a heap-allocated null-terminated copy of
 * the first `len` bytes of `s`.  Caller is responsible for free().
 */
char *scar_alloc_copy(const char *s, size_t len)
{
    char *buf = malloc(len + 1);
    if (!buf)
        return NULL;
    memcpy(buf, s, len);
    buf[len] = '\0';
    return buf;
}
