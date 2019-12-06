test: zen2cc/zen2cc
	./zen2cc/zen2cc

zen2cc/zen2cc: zen2cc/*.c zen2cc/*.h
	$(CC) -o zen2cc/zen2cc zen2cc/*.c

clean:
	rm zen2cc/zen2cc
