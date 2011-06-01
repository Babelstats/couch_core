# This BSD Makefile builds icu4c statically

CFLAGS=	-g -O2 -fPIC
CXXFLAGS=	-fPIC 
LDFLAGS=
MAKE_ENV=	-j3

TAR?=	tar
GUNZIP?=	gunzip
CC?=	gcc
CXX?=	g++
GNUMAKE?=	gmake

SYSTEM_DIR=	${.CURDIR}/system
LIB_DIR=	${SYSTEM_DIR}/lib
ICU_DIR=	${.CURDIR}/icu

ICU_VER=4_6_1

CONFIGURE_ENV=	--disable-debug \
				--enable-static \
				--disable-shared \
				--disable-icuio \
				--disable-layout \
				--disable-extras \
				--disable-tests \
				--disable-samples \
				--prefix=${SYSTEM_DIR}

all: ${LIB_DIR}/libicuuc.a

${LIB_DIR}/libicuuc.a:
	${GUNZIP} -c icu4c-${ICU_VER}-src.tgz | ${TAR} xf -
	for P in ${.CURDIR}/patches/*.patch; do \
	 	(patch -p0 -i $${P} || echo "skipping patch"); \
	done
	(cd ${ICU_DIR}/source && \
	 CC="${CC}" CXX="${CXX}" CXXFLAGS="${CXXFLAGS}" \
	 CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" \
	 ./configure ${CONFIGURE_ENV} && \
	 ${GNUMAKE} ${MAKE_ENV} all ; ${GNUMAKE} install)

clean:
	@rm -rf ${SYSTEM_DIR} ${ICU_DIR}
