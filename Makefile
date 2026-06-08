CC      = clang
CFLAGS  = -Wall -Wextra -g -O0 -Iinclude

SRCS    = src/parse.c src/handler.c src/session.c src/util.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all fuzz fuzz-asan clean

all: scarnet

scarnet: $(OBJS) main.o
	$(CC) $(CFLAGS) $(OBJS) main.o -o scarnet
	@echo "Run: ./scarnet [port]  (default port 4242)"

$(OBJS): %.o: %.c include/scarnet.h
	$(CC) $(CFLAGS) -c $< -o $@

main.o: main.c include/scarnet.h
	$(CC) $(CFLAGS) -c main.c -o main.o

fuzz: $(OBJS) fuzz_target.c
	$(CC) $(CFLAGS) -fsanitize=fuzzer \
	    $(OBJS) fuzz_target.c \
	    -o scarnet_fuzz
	@echo "Run: ./scarnet_fuzz -max_len=4096 corpus/"

fuzz-asan: $(OBJS) fuzz_target.c
	$(CC) $(CFLAGS) -fsanitize=address,undefined,fuzzer \
	    $(OBJS) fuzz_target.c \
	    -o scarnet_fuzz
	@echo "Run: ./scarnet_fuzz -max_len=4096 corpus/"

clean:
	rm -f $(OBJS) main.o scarnet scarnet_fuzz
