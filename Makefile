
install:
	clib install --dev

test:
	@$(CC) test.c src/mapped_file.c $(CFLAGS) -I src -I deps -I deps/greatest -o $@
	@./$@

.PHONY: install test
