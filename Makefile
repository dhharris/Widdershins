CC=clang
CFLAGS=-Wall -Wextra -Werror -Ofast 
LIBS=-lpthread -lcurl -lpython2.7
INCL= -I/usr/include/python2.7

ODIR=obj
SDIR=src

_OBJ=crawler.o queue.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

all: crawler

tsan: CFLAGS += -fsanitize=thread
tsan: debug
debug: clean
debug: CFLAGS += -DDEBUG -g
debug: crawler

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(INCL)

crawler: $(OBJ)
	$(CC) -o $@ $(ODIR)/*.o $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	$(RM) $(ODIR)/*.o crawler
