CC = gcc
CFLAGS = -Wall -I.
SRC = src/main.c
OUT = shell

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)
