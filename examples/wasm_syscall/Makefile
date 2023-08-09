APPLICATION = wasm-syscall
# If no BOARD is defined in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

USEPKG += wamr
USEMODULE += wamr_syscall
USEMODULE += saul_reg 

ifneq ($(BOARD),native)
CFLAGS += -DTHREAD_STACKSIZE_MAIN='(6 * 1024)'
endif

export WAMR_CONFIG := $(abspath config.cmake)

BLOBS += saul.wasm

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

all: saul.wasm

saul.wasm: wasm_sample/saul.c wasm_sample/Makefile
	make -C wasm_sample $@
	mv wasm_sample/$@ .

#######################################################
# Load the rest of the usual RIOT make infrastructure #
#######################################################

include $(RIOTBASE)/Makefile.include
