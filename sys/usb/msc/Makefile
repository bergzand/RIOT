MODULE := usb_msc

ifneq (,$(filter usb_scsi,$(USEMODULE)))
	SRC := msc.c scsi.c
endif

include $(RIOTBASE)/Makefile.base