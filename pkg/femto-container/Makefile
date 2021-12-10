PKG_NAME    = femto-container
PKG_URL     = https://github.com/future-proof-iot/Femto-Container
PKG_VERSION = devel
PKG_LICENSE = apache
PKG_SOURCE_LOCAL = /home/koen/dev/femto-container 

include $(RIOTBASE)/pkg/pkg.mk

all:
	$(QQ)"$(MAKE)" -C $(PKG_SOURCE_DIR)/src -f $(PKG_DIR)/Makefile.femto MODULE=$(PKG_NAME)
