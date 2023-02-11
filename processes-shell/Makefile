PROG = wish
NMPTRN = w*sh
executable = $(wildcard $(NMPTRN))
test-out-dir = $(wildcard tests-*)

$(PROG):
ifeq ($(wildcard $(NMPTRN).c),$(PROG).c)
	gcc -Wall -Werror $(PROG).c -o $@
else
	@echo \"$(PROG).c\" not found.
endif

run: $(PROG)
	./$(PROG)

run-test: test-wish.sh
	@./test-$(PROG).sh -c
	@echo =======Done Testing=======

test-wish.sh:
	@echo "test-$(PROG).sh" not found.

clean:
ifeq ($(executable),$(PROG))
	rm $(PROG)
endif
ifeq ($(test-out-dir),tests-out)
	rm -r tests-out/
endif
