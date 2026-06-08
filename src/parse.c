#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "scarnet.h"

/*
 * parse_cmd — tokenise a single scarnet protocol line into cmd_t.
 *
 * Protocol grammar (newline-terminated text):
 *   SET   <key> <value>
 *   GET   <key>
 *   DEL   <key>
 *   AUTH  <user> <pass>
 *   STATS
 *   FRAG  <id> <offset> <total> <chunk_len>
 *   QUIT
 *
 * Returns 0 on success, -1 on parse error.
 */
int parse_cmd(const char *input, size_t len, cmd_t *out)
{
    char  buf[MAX_MSG_LEN];
    char *tok, *rest;

    if (!input || len == 0 || len >= MAX_MSG_LEN)
        return -1;

    memcpy(buf, input, len);
    buf[len] = '\0';

    /* strip trailing newline */
    size_t blen = strlen(buf);
    if (blen > 0 && buf[blen - 1] == '\n')
        buf[--blen] = '\0';

    memset(out, 0, sizeof(*out));

    tok = strtok_r(buf, " ", &rest);
    if (!tok)
        return -1;
    strncpy(out->verb, tok, sizeof(out->verb) - 1);

    if (strcmp(out->verb, "SET") == 0) {
        tok = strtok_r(NULL, " ", &rest);
        if (!tok) return -1;
        strcpy(out->key, tok);

        tok = strtok_r(NULL, "", &rest);
        if (!tok) return -1;
        strncpy(out->value, tok, sizeof(out->value) - 1);

    } else if (strcmp(out->verb, "GET") == 0 ||
               strcmp(out->verb, "DEL") == 0) {
        tok = strtok_r(NULL, " ", &rest);
        if (!tok) return -1;
        strcpy(out->key, tok);

    } else if (strcmp(out->verb, "AUTH") == 0) {
        tok = strtok_r(NULL, " ", &rest);
        if (!tok) return -1;
        strncpy(out->key, tok, sizeof(out->key) - 1);

        tok = strtok_r(NULL, "", &rest);
        if (!tok) return -1;
        strncpy(out->value, tok, sizeof(out->value) - 1);

    } else if (strcmp(out->verb, "FRAG") == 0) {
        tok = strtok_r(NULL, " ", &rest); if (!tok) return -1;
        out->frag_id = atoi(tok);

        tok = strtok_r(NULL, " ", &rest); if (!tok) return -1;
        out->frag_offset = (size_t)atol(tok);

        tok = strtok_r(NULL, " ", &rest); if (!tok) return -1;
        out->frag_total = (size_t)atol(tok);

        tok = strtok_r(NULL, " ", &rest); if (!tok) return -1;
        out->frag_chunk = (size_t)atol(tok);

    } else if (strcmp(out->verb, "STATS") == 0 ||
               strcmp(out->verb, "QUIT")  == 0) {
        /* no arguments */
    } else {
        return -1;
    }

    return 0;
}

/*
 * parse_batch — allocate and return an array of `count` kv_entry_t
 * records pre-populated from a packed binary blob.
 *
 * Used by the IMPORT bulk-load extension.
 */
kv_entry_t *parse_batch(const char *input, uint32_t count)
{
    kv_entry_t *entries = malloc(count * sizeof(kv_entry_t));
    if (!entries)
        return NULL;
    memset(entries, 0, count * sizeof(kv_entry_t));
    (void)input;
    return entries;
}
