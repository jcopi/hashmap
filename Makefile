CC=gcc
CFLAGS=-I. -O3 -Wall -g -Ifast-hash/

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

fast-hash/%.o: fasthash/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

hashmap_test: hashmap_test.o hashmap.o fast-hash/fasthash.o
	$(CC) -o $@ $^ $(CFLAGS)

hashmap_bench: hashmap_bench.o hashmap.o fast-hash/fasthash.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	-rm -f *.o
	-rm -f fast-hash/*.o