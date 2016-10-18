

PWD :=$(shell pwd)
#export PATH=/opt/buildroot-gcc342/bin:$PATH
CFLAGS = -O2 -fomit-frame-pointer -pipe -I../include -I$(PWD)/lib
LDFLAGS = -Wl,-rpath,/var/lib
LDLIBS = -lp2pnat
STRIPFLAGS = --remove-section=.note --remove-section=.comment

ifeq ($(TARGET),mx60)
CROSS_COMPILE = /opt/arm/usr/bin/arm
LDFLAGS += -L$(PWD)/../lib/mx60
CFLAGS += -DMX60
# LDLIBS = -lmqtt3as -lpthread -lssl -lcrypto
endif
ifeq ($(TARGET),om400)
CROSS_COMPILE = arm-none-linux-gnueabi
LDFLAGS += -L$(PWD)/../lib/om400
CFLAGS += -DOM400
# LDLIBS = -lmqtt3as -lpthread -lssl -lcrypto
endif
ifeq ($(TARGET),mx8)
CROSS_COMPILE = /opt/eldk3/usr/bin/ppc_8xx
LDFLAGS += -L$(PWD)/../lib/mx8
CFLAGS += -DMX8
# LDLIBS = -lmqtt3as -lpthread -lssl -lcrypto
endif
ifeq ($(TARGET),x86)
LDFLAGS += -L$(PWD)/../lib/x86
CFLAGS += -DX86
# LDLIBS = -lmqtt3as -lpthread -lssl -lcrypto
endif
ifeq ($(TARGET),x64)
LDFLAGS += -L$(PWD)/../lib/x64
CFLAGS += -DX86
# LDLIBS = -lmqtt3as -lpthread -lssl -lcrypto
endif
ifeq ($(TARGET),wroc)
CROSS_COMPILE = /opt/buildroot-gcc342/bin/mipsel-linux-uclibc
LDFLAGS += -L$(PWD)/../lib/wroc/
CFLAGS += -DWROC
# LDLIBS = -lmqtt3as -lpthread -lnvram -lssl -lcrypto
endif
ifeq ($(TARGET),)
all:
	@echo -e "\n*** please use ./build.sh ***\n"
endif
CC = ${CROSS_COMPILE}-gcc
STRIP = ${CROSS_COMPILE}-strip
ifeq ($(STATIC),static)
LDFLAGS += -static
endif

p2pdemo: p2pdemo.c comm.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LDLIBS)
	$(STRIP) $(STRIPFLAGS) $@

clean:
	rm -f *.o
	rm -f p2pdemo

