include ../Makefile.tests_common

BLOBS += bpf/memcpy_bpf.bin

USEMODULE += embunit
USEMODULE += bpf

USEMODULE += random
USEMODULE += xtimer
USEMODULE += saul
USEMODULE += saul_reg
USEMODULE += saul_default

CFLAGS += -I$(CURDIR)

include $(RIOTBASE)/Makefile.include
