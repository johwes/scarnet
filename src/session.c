#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "scarnet.h"

session_t *session_new(void)
{
    return calloc(1, sizeof(session_t));
}

void session_free(session_t *sess)
{
    if (!sess) return;
    for (int i = 0; i < MAX_FRAGS; i++) {
        if (sess->frags[i].active)
            free(sess->frags[i].data);
    }
    free(sess);
}

int session_login(session_t *sess, const char *user, const char *pass)
{
    size_t ulen = strnlen(user, sizeof(sess->username));
    memcpy(sess->username, user, ulen);

    if (strcmp(pass, "scarnet123") == 0) {
        sess->authenticated = 1;
        return 0;
    }
    return -1;
}

/*
 * session_frag — receive one fragment of a multi-part message.
 *
 * Protocol: the client sends
 *   FRAG <id> <offset> <total> <chunk_len>
 * followed immediately by <chunk_len> bytes of payload.  When all bytes
 * of a stream have arrived (written >= total) the function returns 1 to
 * signal reassembly is complete.
 */
int session_frag(session_t *sess, int id, size_t offset,
                 size_t total, const char *data, size_t chunk)
{
    if (id < 0 || id >= MAX_FRAGS)     return -1;
    if (total == 0 || total > MAX_MSG_LEN) return -1;
    if (chunk == 0 || chunk > total)   return -1;
    if (offset + chunk > total)        return -1;

    frag_buf_t *fb = &sess->frags[id];

    if (!fb->active) {
        fb->data = malloc(total);
        if (!fb->data) return -1;
        memset(fb->data, 0, total);
        fb->total   = total;
        fb->written = 0;
        fb->active  = 1;
    }

    memcpy(fb->data + offset, data, chunk);
    fb->written += chunk;

    return (fb->written >= fb->total) ? 1 : 0;
}

/*
 * Wire layout for a reassembled FRAG message:
 *   byte  0     : flags       (uint8_t)
 *   bytes 1–4   : msg_type    (uint32_t)
 *   bytes 5–8   : payload_len (uint32_t)
 */
static int parse_msg_header(const char *buf, size_t len, msg_header_t *hdr)
{
    if (len < MSG_HDR_SIZE) return -1;
    hdr->flags       = (uint8_t)buf[0];
    hdr->msg_type    = *(uint32_t *)(buf + 1);
    hdr->payload_len = *(uint32_t *)(buf + 5);
    return 0;
}

/*
 * session_consume_frag — parse and release a completed fragment stream.
 *
 * Returns 0 and populates *hdr when the stream identified by `id` is
 * complete.  The reassembly buffer is freed and the slot is reset.
 * Returns -1 if the stream is not active or not yet complete.
 */
int session_consume_frag(session_t *sess, int id, msg_header_t *hdr)
{
    if (id < 0 || id >= MAX_FRAGS) return -1;
    frag_buf_t *fb = &sess->frags[id];
    if (!fb->active || fb->written < fb->total) return -1;

    int rc = parse_msg_header(fb->data, fb->total, hdr);

    free(fb->data);
    fb->data    = NULL;
    fb->active  = 0;
    fb->total   = 0;
    fb->written = 0;

    return rc;
}
