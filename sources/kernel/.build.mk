KERNEL_C_SRC+=$(wildcard sources/kernel/*.c)
KERNEL_OBJ= \
	$(patsubst sources/%.c, build/%.c.o, $(KERNEL_C_SRC)) \
	$(patsubst sources/%.s, build/%.s.o, $(KERNEL_S_SRC))

KERNEL_BIN=build/kernel.elf

TARGETS += $(KERNEL_BIN)

build/kernel/%.c.o: sources/kernel/%.c
	$(MKCWD)
	$(CC) $(KCFLAGS) -c -o $@ $^

$(KERNEL_BIN): $(KERNEL_OBJ)
	$(MKCWD)
	$(CC) $(KLDFLAGS) $^ -o $@
