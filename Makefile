CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -I. -Iinclude -Ilib/mongoose
SRC = $(wildcard src/*.c) \
	  $(wildcard src/**/*.c) \
	  $(wildcard src/modules/datastructures/*.c) \
	  $(wildcard src/modules/builtins/*.c) \
	  $(wildcard src/modules/autocomplete/*.c) \

SRC := $(filter-out src/modules/builtin.c src/gui/web_main.c, $(SRC))
OUT = shell

WEB_SRC = src/gui/web_main.c
WEB_OUT = moss-web
WEB_HTML = src/gui/web/index.html
WEB_CSS  = src/gui/web/styles.css
WEB_JS   = src/gui/web/main.js

WEB_HDRS = $(WEB_HTML:.html=.html.h) $(WEB_CSS:.css=.css.h) $(WEB_JS:.js=.js.h)

TEST_SRC = $(wildcard tests/*.c)
TEST_BINS = $(TEST_SRC:.c=)
TEST_LIBS = $(filter-out src/main.c, $(SRC))

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

web: $(WEB_OUT)

$(WEB_OUT): $(WEB_SRC) $(WEB_HDRS)
	$(CC) $(CFLAGS) $(WEB_SRC) lib/mongoose/mongoose.c -o $(WEB_OUT)

%.html.h: %.html
	perl scripts/embed.pl $< $(subst .,_,$(notdir $(basename $<)))_html

%.css.h: %.css
	perl scripts/embed.pl $< $(subst .,_,$(notdir $(basename $<)))_css

%.js.h: %.js
	perl scripts/embed.pl $< $(subst .,_,$(notdir $(basename $<)))_js

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
	rm -f $(WEB_HDRS)

scan:
	cppcheck --enable=all --force -Iinclude src/
