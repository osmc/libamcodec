M_PREFIX=$(PREFIX)

TARGET=libamadec.so

TARGET_OBJS  = adec-external-ctrl.o
TARGET_OBJS += adec-internal-mgt.o
TARGET_OBJS += adec-ffmpeg-mgt.c
TARGET_OBJS += adec-message.o
TARGET_OBJS += adec-pts-mgt.o
TARGET_OBJS += feeder.o
TARGET_OBJS += adec_write.c
TARGET_OBJS += adec_read.c
TARGET_OBJS += dsp/audiodsp-ctl.o
TARGET_OBJS += audio_out/alsa-out.o
TARGET_OBJS += audio_out/aml_resample.c
TARGET_OBJS += audiodsp_update_format.c

INSTALL_DIR=${M_PREFIX}/lib

LDFLAGS += -shared -lpthread -lm -lasound -lrt -ldl

CFLAGS+=-O2
CFLAGS+=-I$(shell pwd) -I$(shell pwd)/include -I${PREFIX}/include -I${PREFIX}/usr/include -L${PREFIX}/lib -L${PREFIX}/usr/lib
CFLAGS+=-DALSA_OUT -DENABLE_WAIT_FORMAT

export CC CFLAGS

all: $(TARGET) 

$(TARGET): $(TARGET_OBJS)
	$(CC) $(CFLAGS) $(TARGET_OBJS:%.o=%.c) $(LDFLAGS) -o $@  

install:$(TARGET)
	-install -m 555 ${TARGET} $(INSTALL_DIR)

force:

clean:
	-rm -rf *.o ${AMADEC_DIST} ${MP3LIB} $(PCMLIB) $(ADPCMLIB) $(AACLIB) $(A52LIB)
