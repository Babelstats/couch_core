# This Makefile builds icu4c statically
CFLAGS = -g -O2 -fPIC
CXXFLAGS = -fPIC 
LDFLAGS = 
MAKE_ENV = -j 3

TAR = tar
GUNZIP = gunzip
UNZIP_BIN = unzip
CC = gcc
CXX = g++

SYSTEM_DIR := $(CURDIR)/system
LIB_DIR := $(SYSTEM_DIR)/lib

ICU_DIR := $(CURDIR)/icu

ICU_VER=4_8
ICUDATA=icudt48l

CONFIGURE_ENV = --disable-debug \
		--disable-shared \
	 	--enable-static \
		--disable-icuio \
		--disable-layout \
		--disable-extras \
		--disable-tests \
		--disable-samples \
		--prefix=$(SYSTEM_DIR)

all: $(LIB_DIR)/libicuuc.a

$(LIB_DIR)/libicuuc.a:
	${GUNZIP} -c icu4c-${ICU_VER}-src.tgz | ${TAR} xf -
	$(UNZIP_BIN) $(CURDIR)/$(ICUDATA).zip
	cp $(CURDIR)/$(ICUDATA).dat $(ICU_DIR)/source/data/in/
	for P in $(CURDIR)/patches/*.patch; do \
	 	(patch -p0 -i $${P} || echo "skipping patch"); \
	done
	(cd $(ICU_DIR)/source && \
	 CC="$(CC)" CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" \
	 CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" \
	 ./configure $(CONFIGURE_ENV) && \
	 $(MAKE) $(MAKE_ENV) all ; $(MAKE) install)

clean:
	@rm -rf $(SYSTEM_DIR) $(ICU_DIR)
	@rm -f $(CURDIR)/$(ICUDATA).dat

.EXPORT_ALL_VARIABLES:
