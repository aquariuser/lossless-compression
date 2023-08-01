CC = gcc
SRC_DIR = ./
INCLUDEPATH = -I src

MAINSRC = $(notdir $(shell ls $(SRC_DIR)*.c))
OBJ = $(patsubst %.c, %.o, $(MAINSRC))

CFLAGS = -O0 -g -fPIC -Werror

fblcd.out: $(OBJ)
	$(CC) $^ $(CFLAGS) -o $@

$(OBJ): %.o: $(SRC_DIR)%.c
	$(CC) -c $< $(INCLUDEPATH) $(CFLAGS) -o $@

.PHONY: clean
clean :
	rm -f *.out
	rm -f *.o



