CC = gcc
CFLAGS = -Wall -I.
SRC = $(wildcard src/*.c)
OUT = shell

TEST_SRC = $(wildcard tests/*.c)
TEST_BINS = $(TEST_SRC:.c=)

all: $(OUT) test

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

test: $(TEST_BINS)
	@echo "Running all tests..."
	@for test in $(TEST_BINS); do\
		echo "Running $$test...";\
		./$$test || exit 1; \
	done

$(TEST_BINS): $(TEST_SRC)
	$(CC) $(CFLAGS) $< -o $@ -lcmocka

.PHONY: all test clean

clean:
	rm -f $(OUT)
	rm -f $(TEST_BINS)
