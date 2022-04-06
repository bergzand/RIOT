MODULE = sphss

SRC += fips180.c
SRC += fips202.c
SRC += hash.c
SRC += hss.c
SRC += lms.c
SRC += lms_ots.c
SRC += lms_utils.c
SRC += utils.c
SRC += randombytes.c

CFLAGS += -Wno-sign-compare

include $(RIOTBASE)/Makefile.base
