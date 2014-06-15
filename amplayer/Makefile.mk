
ROOTFS?=$(PREFIX)

CROSS=$(CC:%gcc=%)

QUITE_CMD?=no

INSTALL_FLAGS=-m 755

CPP=$(CROSS)g++
AS=$(CROSS)as
AR=$(CROSS)ar
LD=$(CROSS)ld
NM=$(CROSS)nm
STRIP=$(CROSS)strip
OBJCOPY=$(CROSS)objcopy
export CC CPP AS AR LD NM STRIP OBJCOPY

TOPDIR=$(shell pwd)
SRC?=$(TOPDIR)
SRCTREE=$(SRC)
BUILDTREE=$(SRC)/out

ifeq ($(QUITE_CMD),yes)
Q=@
endif	
export Q

TARGET =libamplayer.so


TARGET_IS_LIB=$(TARGET:%so=yes)



LDFLAGS+= -L$(PREFIX)/lib -lavutil -lavformat -lswscale -lavcodec -lavutil -lm  -lpthread -lamcodec -lamavutils

INSTALL_DIR?=$(PREFIX)/lib
LDFLAGS+=-shared 
CFLAGS=$(DIRS:%/=-I$(SRC)/%/include) 


DIRS  = player/
DIRS += player/system/

CFLAGS+=-I${SRCTREE}/player -I${SRCTREE}/../amffmpeg -I${SRCTREE}/../amcodec/include -I${SRCTREE}/../amadec/include -I${SRCTREE}/../amavutils/include
CFLAGS+=-fPIC -DENABLE_FREE_SCALE
target_all=  $(TARGET)



INCLUDE=${SRCTREE}/include/

CFLAGS+=-Wall 
#CFLAGS+=-Werror

#CFLAGS+=-O0  -gdwarf-2  -g
CFLAGS+=-O2





all:$(target_all)

dirs=$(DIRS)

DIROBJ=$(dirs:%/=%-dir)


$(target_all):$(DIROBJ) 
	$(call ld_mk,$(OUT_DIR)$@,$^)



	

include $(SRCTREE)/rules.mk
export CC CPP AS AR LD TOPDIR SRCTREE  CFLAGS LDFLAGS Q

CLRDIR=$(dirs:%/=%-clrdir)
clean:$(CLRDIR)
	rm -rf $(target_all)

install:$(target_all)
	-install $(INSTALL_FLAGS)	$(target_all)  $(INSTALL_DIR)
	mkdir -p $(PREFIX)/include/amplayer
	-install $(INSTALL_FLAGS) $(DIRS:%/=$(SRC)/%/include/*)  $(PREFIX)/include/amplayer

