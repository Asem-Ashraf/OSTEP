executable = $(wildcard w*sh)
test-out-dir = $(wildcard tests-*)
build:
	gcc -Wall -Werror wish.c -o wish
run:
	./wish
run-test:
	./test-wish.sh -c
clean:
ifeq ($(executable),wish)
	rm wish
endif
ifeq ($(test-out-dir),tests-out)
	rm -r tests-out/
endif
