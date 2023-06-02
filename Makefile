CONFIG_MODULE_SIG = n
TARGET_MODULE := fibdrvko

obj-m := $(TARGET_MODULE).o
$(TARGET_MODULE)-objs := fibdrv.o bn.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

GIT_HOOKS := .git/hooks/applied

all: $(GIT_HOOKS) client test
	$(MAKE) -C $(KDIR) M=$(PWD) modules

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) client out test
load:
	sudo insmod $(TARGET_MODULE).ko
unload:
	sudo rmmod $(TARGET_MODULE) || true >/dev/null

test: test.c
	$(CC) -o $@ $^

client: client.c
	$(CC) -o $@ $^

PRINTF = env printf
PASS_COLOR = \e[32;01m
NO_COLOR = \e[0m
pass = $(PRINTF) "$(PASS_COLOR)$1 Passed [-]$(NO_COLOR)\n"

check: all
	$(MAKE) unload
	$(MAKE) load
	sudo ./client > out
	$(MAKE) unload
	@diff -u out scripts/expected.txt && $(call pass)
	@scripts/verify.py

plot: all
	$(MAKE) unload
	$(MAKE) load
	@rm -f fast.txt naive.txt perf.png
	sudo ./test > fast.txt
	sudo ./test n > naive.txt
	$(MAKE) unload
	@gnuplot scripts/plot.gp
	@eog perf.png