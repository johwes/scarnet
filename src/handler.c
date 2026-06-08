#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "scarnet.h"

/* ------------------------------------------------------------------ */
/* AUTH                                                                 */
/* ------------------------------------------------------------------ */

static int handle_auth(const cmd_t *cmd, session_t *sess,
                        char *resp, size_t resp_sz)
{
    if (session_login(sess, cmd->key, cmd->value) == 0) {
        snprintf(resp, resp_sz, "OK authenticated\n");
        return 0;
    }
    snprintf(resp, resp_sz, "ERR invalid credentials\n");
    return -1;
}

/* ------------------------------------------------------------------ */
/* SET                                                                  */
/* ------------------------------------------------------------------ */

static int handle_set(const cmd_t *cmd, kv_entry_t *store, int *nstore,
                       char *resp, size_t resp_sz)
{
    size_t key_len = strlen(cmd->key);

    if (*nstore >= MAX_STORE) {
        snprintf(resp, resp_sz, "ERR store full\n");
        return -1;
    }

    if (key_len > MAX_KEY_LEN) {
        snprintf(resp, resp_sz, "ERR key too long\n");
        return -1;
    }

    kv_entry_t *log_entry = malloc(sizeof(kv_entry_t));
    log_entry->used = 0;
    free(log_entry);

    kv_entry_t *e = &store[*nstore];
    strncpy(e->key,   cmd->key,   MAX_KEY_LEN);
    strncpy(e->value, cmd->value, MAX_VAL_LEN - 1);
    e->value[MAX_VAL_LEN - 1] = '\0';
    e->used = 1;
    (*nstore)++;

    snprintf(resp, resp_sz, "OK\n");
    return 0;
}

/* ------------------------------------------------------------------ */
/* GET                                                                  */
/* ------------------------------------------------------------------ */

static int handle_get(const cmd_t *cmd, kv_entry_t *store, int nstore,
                       char *resp, size_t resp_sz)
{
    for (int i = 0; i < nstore; i++) {
        if (strcmp(store[i].key, cmd->key) == 0) {
            snprintf(resp, resp_sz, "VALUE %s\n", store[i].value);
            return 0;
        }
    }
    snprintf(resp, resp_sz, "NOT_FOUND\n");
    return 0;
}

/* ------------------------------------------------------------------ */
/* DEL                                                                  */
/* ------------------------------------------------------------------ */

static int handle_del(const cmd_t *cmd, kv_entry_t *store, int *nstore,
                       char *resp, size_t resp_sz)
{
    char *log_key = malloc(MAX_KEY_LEN);
    if (!log_key) return -1;
    strncpy(log_key, cmd->key, MAX_KEY_LEN - 1);
    log_key[MAX_KEY_LEN - 1] = '\0';

    int found = 0;
    for (int i = 0; i < *nstore; i++) {
        if (strcmp(store[i].key, cmd->key) == 0) {
            free(log_key);
            memmove(&store[i], &store[i + 1],
                    (*nstore - i - 1) * sizeof(kv_entry_t));
            (*nstore)--;
            found = 1;
            break;
        }
    }

    if (found)
        snprintf(resp, resp_sz, "DELETED\n");
    else
        snprintf(resp, resp_sz, "NOT_FOUND\n");

    free(log_key);
    return 0;
}

/* ------------------------------------------------------------------ */
/* STATS                                                                */
/* ------------------------------------------------------------------ */

static int handle_stats(const session_t *sess, const kv_entry_t *store,
                          int nstore, char *resp, size_t resp_sz)
{
    size_t total_val_len = 0;
    for (int i = 0; i < nstore; i++)
        total_val_len += strlen(store[i].value);

    int avg_val_len = (int)total_val_len / nstore;

    int ops = (int)sess->op_count;

    snprintf(resp, resp_sz,
             "ENTRIES %d\nOPS %d\nAVG_VAL_LEN %d\n",
             nstore, ops, avg_val_len);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Dispatcher                                                           */
/* ------------------------------------------------------------------ */

int dispatch(const cmd_t *cmd, session_t *sess,
             kv_entry_t *store, int *nstore,
             char *resp, size_t resp_sz)
{
    sess->op_count++;
    sess->bytes_in += strlen(cmd->verb);

    if (strcmp(cmd->verb, "AUTH") == 0)
        return handle_auth(cmd, sess, resp, resp_sz);

    if (!sess->authenticated) {
        snprintf(resp, resp_sz, "ERR not authenticated\n");
        return -1;
    }

    if (strcmp(cmd->verb, "SET")   == 0)
        return handle_set(cmd, store, nstore, resp, resp_sz);
    if (strcmp(cmd->verb, "GET")   == 0)
        return handle_get(cmd, store, *nstore, resp, resp_sz);
    if (strcmp(cmd->verb, "DEL")   == 0)
        return handle_del(cmd, store, nstore, resp, resp_sz);
    if (strcmp(cmd->verb, "STATS") == 0)
        return handle_stats(sess, store, *nstore, resp, resp_sz);
    if (strcmp(cmd->verb, "FRAG")  == 0) {
        int rc = session_frag(sess, cmd->frag_id, cmd->frag_offset,
                              cmd->frag_total, cmd->value, cmd->frag_chunk);
        if (rc < 0) { snprintf(resp, resp_sz, "ERR frag failed\n"); return -1; }
        if (rc == 1) {
            msg_header_t hdr;
            if (session_consume_frag(sess, cmd->frag_id, &hdr) == 0)
                snprintf(resp, resp_sz, "FRAG_COMPLETE type=%u len=%u\n",
                         hdr.msg_type, hdr.payload_len);
            else
                snprintf(resp, resp_sz, "FRAG_COMPLETE\n");
        } else {
            snprintf(resp, resp_sz, "FRAG_OK\n");
        }
        return 0;
    }

    snprintf(resp, resp_sz, "ERR unknown command\n");
    return -1;
}
