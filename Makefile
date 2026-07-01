CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -I. -Iinclude -Ilib/mongoose
SRC = $(wildcard src/*.c) \
	  $(wildcard src/**/*.c) \
	  $(wildcard src/modules/datastructures/*.c) \
	  $(wildcard src/modules/builtins/*.c) \
	  $(wildcard src/modules/autocomplete/*.c) \

SRC := $(filter-out src/modules/builtin.c, $(SRC))
OUT = shell

WEB_SRC = src/gui/web/web_main.c
WEB_OUT = moss-web

TEST_SRC = $(wildcard tests/*.c)
TEST_BINS = $(TEST_SRC:.c=)
TEST_LIBS = $(filter-out src/main.c, $(SRC))

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

web: $(WEB_OUT)

$(WEB_OUT): $(WEB_SRC)
	$(CC) $(CFLAGS) $(WEB_SRC) lib/mongoose/mongoose.c -o $(WEB_OUT) -lws2_32

test: $(TEST_BINS)
	@echo "Running all tests..."
	@for test in $(TEST_BINS); do\
		echo "Running $$test...";\
		./$$test || exit 1; \
	done

$(TEST_BINS): tests/%: tests/%.c $(TEST_LIBS)
	$(CC) $(CFLAGS) $< $(TEST_LIBS) -o $@ -lcmocka

.PHONY: all test clean scan check-mem web

clean:
	rm -f $(OUT)
	rm -f $(TEST_BINS)
	rm -f $(WEB_OUT)

scan:
	cppcheck --enable=all --force -Iinclude src/
