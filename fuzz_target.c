/*
 * fuzz_target.c — libFuzzer entry point for scarnet.
 *
 * Build with:
 *   make fuzz-asan
 *
 * Run:
 *   ./scarnet_fuzz -max_len=4096 corpus/
 *
 * The fuzzer exercises parse_cmd + dispatch with a persistent session,
 * so state accumulates across calls — multi-step protocol sequences
 * (e.g. AUTH followed by FRAG commands) are reachable.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "scarnet.h"

static kv_entry_t store[MAX_STORE];
static int        nstore = 0;
static session_t *sess   = NULL;

__attribute__((constructor))
static void init(void)
{
    sess = session_new();
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0 || size > MAX_MSG_LEN)
        return 0;

    cmd_t cmd;
    char  resp[512];

    if (parse_cmd((const char *)data, size, &cmd) != 0)
        return 0;

    dispatch(&cmd, sess, store, &nstore, resp, sizeof(resp));
    return 0;
}
