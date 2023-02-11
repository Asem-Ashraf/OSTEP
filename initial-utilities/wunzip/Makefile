PROG= wunzip
executable = $(wildcard w*nzip)
test-out-dir = $(wildcard tests-*)
$(PROG): $(wildcard w*.c)
	gcc -Wall -Werror $? -o $@
run-test: $(PROG)
	./test-$^.sh -c
clean:
ifeq ($(executable),$(PROG))
	rm $(PROG)
endif
ifeq ($(test-out-dir),tests-out)
	rm -r tests-out/
endif
