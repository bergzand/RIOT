include ../Makefile.tests_common

USEMODULE += embunit
USEMODULE += random
USEMODULE += fmt

USEPKG += sphss
CFLAGS += -DLMOTS_SHA256_N32_W8 -DLMS_SHA256_M32_H5
CFLAGS += -DTHREAD_STACKSIZE_MAIN=8192

include $(RIOTBASE)/Makefile.include
