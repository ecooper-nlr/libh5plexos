CFLAGS=-static -std=gnu11 -Wall

h5plexos: src/h5plexos_cli.o lib/h5plexos.o lib/plexostables.o lib/parsexml.o lib/makehdf5.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/h5plexos_cli.o
	rm -f lib/h5plexos.o
	rm -f lib/plexostables.o
	rm -f lib/parsexml.o
	rm -f lib/makehdf5.o
