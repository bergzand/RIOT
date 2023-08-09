include ../Makefile.tests_common

BLOBS += rbpf/fletcher32_rbpf.bin
BLOBS += rbpf/random_num.bin
BLOBS += rbpf/coap.bin

USEMODULE += embunit
USEMODULE += rbpf
USEMODULE += gcoap
USEMODULE += gnrc_ipv6_default
USEMODULE += netdev_default
USEMODULE += auto_init_gnrc_netif

pkg-prepare: rbpf/$(RBPF_BINS)

RBPF_SOURCES ?= $(wildcard rbpf/*.c)
RBPF_BINS = $(RBPF_SOURCES:.c=.bin)

$(RBPF_BINS): %.bin:%.c
	$(MAKE) -C $(CURDIR)/rbpf

all: $(RBPF_BINS)
include $(RIOTBASE)/Makefile.include
