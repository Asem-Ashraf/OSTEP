PROG = wzip
NMPTRN = w*ip
TSTSCRPT = test-$(PROG).sh
TSTPTRN = test-*.sh
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

run-test: $(PROG)
ifeq ($(wildcard $(TSTPTRN)),$(TSTSCRPT))
	@./$(TSTSCRPT) -c
	@echo =======Done Testing=======
else
	@echo \"$(TSTSCRPT)\" not found.
endif

clean:
ifeq ($(executable),$(PROG))
	rm $(PROG)
endif
ifeq ($(test-out-dir),tests-out)
	rm -r tests-out/
endif
