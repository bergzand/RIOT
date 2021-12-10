PKG_NAME    = femto-container
PKG_URL     = https://github.com/future-proof-iot/Femto-Container
PKG_VERSION = d27568f233c49c3be3ea5f418373636f1ce0f496
PKG_LICENSE = apache

include $(RIOTBASE)/pkg/pkg.mk

all:
	$(QQ)"$(MAKE)" -C $(PKG_SOURCE_DIR)/src -f $(PKG_DIR)/Makefile.femto MODULE=$(PKG_NAME)
