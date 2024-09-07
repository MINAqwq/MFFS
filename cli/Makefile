CC     = clang
CFLAGS = -std=c89 -Wall -Wextra -Wno-unused-function -O3 -fpic -march=native \
	-DMFFSCLI_VERSION=\"1\" \
	-DGIT_HASH=\"$(shell git rev-parse --short HEAD)\"

OBJ =    \
driver.o \
main.o   \
string.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

mffscli: $(OBJ)
	$(CC) $(OBJ) -o $@

.PHONY:
clean:
	rm $(OBJ) mffscli
