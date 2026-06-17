CFLAGS=-static -std=gnu11 -Wall
BINARY=build/h5plexos

$(BINARY): src/h5plexos_cli.o lib/h5plexos.o lib/plexostables.o lib/parsexml.o lib/makehdf5.o
	mkdir -p build
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

h5plexos: $(BINARY)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/h5plexos_cli.o
	rm -f lib/h5plexos.o
	rm -f lib/plexostables.o
	rm -f lib/parsexml.o
	rm -f lib/makehdf5.o
	rm -f $(BINARY)
