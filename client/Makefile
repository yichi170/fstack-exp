ifeq ($(FF_PATH),)
        FF_PATH=${TOPDIR}
endif

ifneq ($(shell pkg-config --exists libdpdk && echo 0),0)
$(error "No installation of DPDK found, maybe you should export environment variable `PKG_CONFIG_PATH`")
endif

PKGCONF ?= pkg-config

CFLAGS += -O0 -g -gdwarf-2 $(shell $(PKGCONF) --cflags libdpdk)

LIBS+= $(shell $(PKGCONF) --static --libs libdpdk)
LIBS+= -L${FF_PATH}/lib -Wl,--whole-archive,-lfstack,--no-whole-archive
LIBS+= -Wl,--no-whole-archive -lrt -lm -ldl -lcrypto -pthread -lnuma

TARGET=client

all:
	gcc ${CFLAGS} -o ff${TARGET} ff${TARGET}.c ${LIBS}
	gcc ${CFLAGS} -o ${TARGET} ${TARGET}.c ${LIBS}

.PHONY: clean
clean:
	rm -f ${TARGET} ff${TARGET}
