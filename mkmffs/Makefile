CC     = clang
CFLAGS = -std=c89 -Wall -Wextra -Wno-unused-function -O3 -fpic -march=native -DMKMFFS_VERSION=\"1.0\"

OBJ =    \
format.o \
main.o   \
string.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

mkmffs: $(OBJ)
	$(CC) $(OBJ) -o $@

.PHONY:
clean:
	rm $(OBJ) mkmffs
