CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -I. -Iinclude
SRC = $(wildcard src/*.c) \
	  $(wildcard src/**/*.c) \
	  $(wildcard src/modules/datastructures/*.c) \
	  $(wildcard src/modules/builtins/*.c) \

SRC := $(filter-out src/modules/builtin.c, $(SRC))
OUT = shell

TEST_SRC = $(wildcard tests/*.c)
TEST_BINS = $(TEST_SRC:.c=)
TEST_LIBS = $(filter-out src/main.c, $(SRC))

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

test: $(TEST_BINS)
	@echo "Running all tests..."
	@for test in $(TEST_BINS); do\
		echo "Running $$test...";\
		./$$test || exit 1; \
	done

$(TEST_BINS): tests/%: tests/%.c $(TEST_LIBS)
	$(CC) $(CFLAGS) $< $(TEST_LIBS) -o $@ -lcmocka

.PHONY: all test clean scan check-mem

clean:
	rm -f $(OUT)
	rm -f $(TEST_BINS)

scan:
	cppcheck --enable=all --force -Iinclude src/

check-mem:
	@echo "Running memory checks with valgrind..."
	@for test in $(TEST_BINS); do \
		echo "Checking $$test..."; \
		valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$$test || exit 1; \
	done
