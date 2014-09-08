TARGET = libamavutils.so

TARGET_OBJS=amaudioutils.o amconfigutils.o amdisplayutils.o ammodule.o amsysfsutils.o Amvideoutils.o itemlist.o\

LDFLAGS += -shared -lm -lrt

TARGET_DIR=$(shell pwd)/../../../target
STAGING_DIR=$(shell pwd)/../../../staging

CFLAGS+=-O2 -fPIC
CFLAGS+=-I$(shell pwd) -I$(shell pwd)/include -I$(shell pwd)/../amcodec/include -I${STAGING_DIR}/include -I${STAGING_DIR}/usr/include -L${TARGET_DIR}/lib -L${TARGET_DIR}/usr/lib

export CC CFLAGS

all: $(TARGET) 

$(TARGET): $(TARGET_OBJS)
	echo "CC=$(CC)"
	echo "LD=$(LD)"
	echo "CFLAGS=$(CFLAGS)"
	echo "LDFLAGS=$(LDFLAGS)"
	$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET_OBJS:%.o=%.c) -o $@  

clean:
	-rm -rf *.o $(TARGET)