PKG_NAME=pytorch
PKG_URL=https://github.com/pytorch/pytorch
PKG_VERSION=71f889c7d265b9636b93ede9d651c0a9c4bee191 # 1.10.2
PKG_LICENSE=BSD

include $(RIOTBASE)/pkg/pkg.mk

all: $(PKG_SOURCE_DIR)/
	$(QQ)"$(MAKE)" -C $(PKG_SOURCE_DIR) -f $(RIOTBASE)/Makefile.base MODULE=$(PKG_NAME)
