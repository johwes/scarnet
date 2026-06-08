#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "scarnet.h"

#define DEFAULT_PORT 4242
#define BACKLOG      8

static kv_entry_t store[MAX_STORE];
static int        nstore = 0;

static void handle_client(int fd)
{
    session_t *sess = session_new();
    if (!sess) { close(fd); return; }

    FILE *in  = fdopen(fd, "r");
    if (!in)  { session_free(sess); close(fd); return; }
    FILE *out = fdopen(dup(fd), "w");
    if (!out) { session_free(sess); fclose(in); return; }

    char line[MAX_MSG_LEN];
    char resp[MAX_MSG_LEN];

    fprintf(out, "scarnet %s ready\n", SCARNET_VERSION);
    fflush(out);

    while (fgets(line, sizeof(line), in)) {
        size_t len = strlen(line);
        scar_log(line);

        cmd_t cmd;
        if (parse_cmd(line, len, &cmd) != 0) {
            fprintf(out, "ERR parse error\n");
            fflush(out);
            continue;
        }

        if (strcmp(cmd.verb, "QUIT") == 0) {
            fprintf(out, "OK bye\n");
            fflush(out);
            break;
        }

        dispatch(&cmd, sess, store, &nstore, resp, sizeof(resp));
        fputs(resp, out);
        fflush(out);
    }

    fclose(in);
    fclose(out);
    session_free(sess);
}

int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    if (argc > 1)
        port = atoi(argv[1]);

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); return 1; }

    int one = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port        = htons((uint16_t)port),
    };
    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    if (listen(srv, BACKLOG) < 0) { perror("listen"); return 1; }

    printf("scarnet %s listening on port %d\n", SCARNET_VERSION, port);

    for (;;) {
        struct sockaddr_in cli;
        socklen_t cli_len = sizeof(cli);
        int fd = accept(srv, (struct sockaddr *)&cli, &cli_len);
        if (fd < 0) { perror("accept"); continue; }
        printf("[+] %s connected\n", inet_ntoa(cli.sin_addr));
        handle_client(fd);
        printf("[-] connection closed\n");
    }
}
