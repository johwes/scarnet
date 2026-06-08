#ifndef SCARNET_H
#define SCARNET_H

#include <stddef.h>
#include <stdint.h>

#define SCARNET_VERSION  "0.1.0"

#define MAX_KEY_LEN   64
#define MAX_VAL_LEN   256
#define MAX_STORE     128
#define MAX_FRAGS     16
#define MAX_MSG_LEN   4096

/* Wire header for a reassembled FRAG message:
 *   byte  0      : flags       (uint8_t)
 *   bytes 1–4    : msg_type    (uint32_t)
 *   bytes 5–8    : payload_len (uint32_t)
 */
#define MSG_HDR_SIZE  9

typedef struct {
    uint8_t  flags;
    uint32_t msg_type;
    uint32_t payload_len;
} msg_header_t;

/* ------------------------------------------------------------------ */
/* Data types                                                           */
/* ------------------------------------------------------------------ */

/* One entry in the key-value store */
typedef struct {
    char key[MAX_KEY_LEN];
    char value[MAX_VAL_LEN];
    int  used;
} kv_entry_t;

/* Fragment reassembly buffer for a single FRAG stream */
typedef struct {
    char   *data;     /* heap-allocated reassembly buffer */
    size_t  total;    /* expected total message length    */
    size_t  written;  /* bytes written so far             */
    int     active;   /* 1 = slot in use                  */
} frag_buf_t;

/* Per-connection session */
typedef struct {
    char       username[32];
    int        authenticated;
    size_t     op_count;         /* total commands dispatched this session */
    size_t     bytes_in;         /* total bytes received this session      */
    frag_buf_t frags[MAX_FRAGS];
} session_t;

/* Parsed command */
typedef struct {
    char   verb[16];
    char   key[MAX_KEY_LEN];
    char   value[MAX_VAL_LEN];
    int    frag_id;
    size_t frag_offset;
    size_t frag_total;
    size_t frag_chunk;
} cmd_t;

/* ------------------------------------------------------------------ */
/* parse.c                                                              */
/* ------------------------------------------------------------------ */
int         parse_cmd(const char *input, size_t len, cmd_t *out);
kv_entry_t *parse_batch(const char *input, uint32_t count);

/* ------------------------------------------------------------------ */
/* handler.c                                                            */
/* ------------------------------------------------------------------ */
int dispatch(const cmd_t *cmd, session_t *sess,
             kv_entry_t *store, int *nstore,
             char *resp, size_t resp_sz);

/* ------------------------------------------------------------------ */
/* session.c                                                            */
/* ------------------------------------------------------------------ */
session_t *session_new(void);
void       session_free(session_t *sess);
int        session_login(session_t *sess, const char *user, const char *pass);
int        session_frag(session_t *sess, int id, size_t offset,
                        size_t total, const char *data, size_t chunk);
int        session_consume_frag(session_t *sess, int id, msg_header_t *hdr);

/* ------------------------------------------------------------------ */
/* util.c                                                               */
/* ------------------------------------------------------------------ */
void  scar_log(const char *msg);
int   scar_atoi(const char *s, int *out);
char *scar_alloc_copy(const char *s, size_t len);

#endif /* SCARNET_H */
