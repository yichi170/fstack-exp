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

TARGET=server2

all: ff_epoll_server.c
	# gcc ${CFLAGS} -o ff${TARGET} ffserver2.c -DFSTACK ${LIBS}
	# gcc ${CFLAGS} -o ${TARGET} ${TARGET}.c ${LIBS}
	gcc ${CFLAGS} -o ff_epoll_server ff_epoll_server.c -DFSTACK ${LIBS}
	gcc -o epoll_server ff_epoll_server.c
.PHONY: clean
clean:
	rm -f ${TARGET}
