CFLAGS=-g

test: zen2cc/zen2cc
	./zen2cc/zen2cc -p tests/expr.zen

test_token: zen2cc/zen2cc
	@for f in tests/*.token; do \
		printf "Testing $${f##*/} ... "; \
		./zen2cc/zen2cc -t "$${f%.*}.zen" > "$${f%.*}.temp"; \
		DIFF="$$(diff -q "$${f%.*}.temp" "$$f")"; \
		if [ -z "$$DIFF" ]; \
		then printf "OK\n"; \
		rm  "$${f%.*}.temp"; \
		else printf "FAILED\n"; \
		diff  "$${f%.*}.temp" "$$f"; \
		fi; \
	done

test_token_update:
	@for f in tests/*.zen; do \
		printf "Updating $${f##*/} ... \n"; \
		./zen2cc/zen2cc -t "$$f" > "$${f%.*}.token"; \
	done

test_parse: zen2cc/zen2cc
	@for f in tests/*.parse; do \
		printf "Testing $${f##*/} ... "; \
		./zen2cc/zen2cc -p "$${f%.*}.zen" > "$${f%.*}.temp"; \
		DIFF="$$(diff -q "$${f%.*}.temp" "$$f")"; \
		if [ -z "$$DIFF" ]; \
		then printf "OK\n"; \
		rm  "$${f%.*}.temp"; \
		else printf "FAILED\n"; \
		diff  "$${f%.*}.temp" "$$f"; \
		fi; \
	done

test_parse_update:
	@for f in tests/*.zen; do \
		printf "Updating $${f##*/} ... \n"; \
		./zen2cc/zen2cc -p "$$f" > "$${f%.*}.parse"; \
	done

zen2cc/zen2cc: zen2cc/*.c zen2cc/*.h
	$(CC) $(CFLAGS) -o zen2cc/zen2cc zen2cc/*.c

clean:
	rm -f zen2cc/zen2cc tests/*.temp
