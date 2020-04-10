test: zen2cc/zen2cc
	./zen2cc/zen2cc -t tests/ops.zen
	#./zen2cc/zen2cc -p tests/syntax.zen

test_token: zen2cc/zen2cc
	@for f in tests/*.zen; do \
		printf "Testing $${f##*/} ... "; \
		./zen2cc/zen2cc -t "$$f" > "$${f%.*}.temp"; \
		DIFF="$$(diff -q "$${f%.*}.temp" "$${f%.*}.token")"; \
		if [ -z "$$DIFF" ]; \
		then printf "OK\n"; \
		rm  "$${f%.*}.temp"; \
		else printf "FAILED\n"; \
		diff  "$${f%.*}.temp" "$${f%.*}.token"; \
		fi; \
	done

test_token_update:
	@for f in tests/*.zen; do \
		printf "Updating $${f##*/} ... \n"; \
		./zen2cc/zen2cc -t "$$f" > "$${f%.*}.token"; \
	done

zen2cc/zen2cc: zen2cc/*.c zen2cc/*.h
	$(CC) -o zen2cc/zen2cc zen2cc/*.c

clean:
	rm -f zen2cc/zen2cc tests/*.temp
