/*
 * harness_dispatch.c — libFuzzer harness for scarnet dispatch()
 *
 * Bugs targeted:
 *   - handle_del: double-free of log_key (freed on match inside loop,
 *     then freed again unconditionally after the loop)
 *   - handle_stats: divide-by-zero (total_val_len / nstore when nstore == 0)
 *
 * Strategy: authenticate once with hardcoded credentials, then feed the
 * fuzzed input as a protocol command to dispatch().  The fuzzer can explore
 * all authenticated command paths (SET, GET, DEL, STATS, FRAG) freely.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "scarnet.h"

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    if (Size == 0)
        return 0;

    session_t  *sess   = session_new();
    if (!sess) return 0;

    kv_entry_t  store[MAX_STORE];
    int         nstore = 0;
    char        resp[512];

    /* Authenticate once — hardcode the credential gate so the fuzzer
     * reaches all authenticated handlers without guessing the password. */
    cmd_t auth_cmd;
    memset(&auth_cmd, 0, sizeof(auth_cmd));
    strncpy(auth_cmd.verb,  "AUTH",       sizeof(auth_cmd.verb)  - 1);
    strncpy(auth_cmd.key,   "fuzzuser",   sizeof(auth_cmd.key)   - 1);
    strncpy(auth_cmd.value, "scarnet123", sizeof(auth_cmd.value) - 1);
    dispatch(&auth_cmd, sess, store, &nstore, resp, sizeof(resp));

    /* Feed fuzz input as a null-terminated protocol line */
    char *line = malloc(Size + 2);
    if (!line) { session_free(sess); return 0; }
    memcpy(line, Data, Size);
    line[Size]     = '\n';
    line[Size + 1] = '\0';

    cmd_t cmd;
    if (parse_cmd(line, Size + 1, &cmd) == 0)
        dispatch(&cmd, sess, store, &nstore, resp, sizeof(resp));

    free(line);
    session_free(sess);
    return 0;
}
