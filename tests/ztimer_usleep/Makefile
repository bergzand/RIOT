include ../Makefile.tests_common

USEMODULE += ztimer_usec
USEMODULE += dbgpin

CFLAGS += -DDBGPIN_PARAMS=GPIO_PIN\(0,11\),GPIO_PIN\(0,12\),GPIO_PIN\(0,13\),GPIO_PIN\(0,14\),GPIO_PIN\(0,15\),GPIO_PIN\(0,16\)

# Port and pin configuration for probing with oscilloscope
# Port number should be found in port enum e.g in cpu/include/periph_cpu.h
#FEATURES_REQUIRED += periph_gpio
#CFLAGS += -DSLEEP_PIN=7
#CFLAGS += -DSLEEP_PORT=PORT_F

include $(RIOTBASE)/Makefile.include
