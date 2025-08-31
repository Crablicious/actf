CC	?= gcc
CFLAGS	?= --std=c11 -Wall -Wpedantic -Wextra -Wvla -Wundef -Wshadow -Wincompatible-pointer-types \
	-g -fsanitize=address,undefined -D_POSIX_C_SOURCE=200809L
CFLAGS	+= -I../
CFLAGS	+= -O3 -flto
LDFLAGS	=

OBJS	=

.PHONY: all
all: libcrust.a test

rb_tree.o: rb_tree.h
benchmark.o: rb_tree.o uival.h ../rng_int.h ../rng.h ival.h
rng.o: ../rng.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^

tests.o: vec.h int_vec.h gvec.h ival.h rb_tree.h uival.h sival.h map.h map_util.h \
	u64tosize.h strtoidx.h arena.h

benchmark: benchmark.o rb_tree.o rng.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

libcrust.a: rb_tree.o
	ar rcs $@ $^

tests.out: tests.o rb_tree.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) `pkg-config --cflags --libs cunit`

.PHONY: test
test: tests.out
	./tests.out

.PHONY: clean
clean:
	$(RM) *.o tests.out benchmark libcrust.a *.png *.dot
