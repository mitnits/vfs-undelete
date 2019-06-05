K_VERSION:=$(shell uname -r | cut -c1-3 | sed 's/2\.[56]/2\.6/')

TARGET:=vfs-undelete.o

EXTRA_CFLAGS := -I$(PWD)/../include
CFILES := device.c  vfs.c

ifeq ($(K_VERSION), 2.6)

obj-m += $(TARGET)
vfs-undelete-objs := $(CFILES:.c=.o)

EXTRA_CFLAGS := -I$(PWD)/../include

# KSRC := /lib/modules/$(shell uname -r)/build
KSRC := /home/muli/vfs-undelete/kernel

all:
	make -C $(KSRC) M=$(PWD) modules

clean:
	make -C $(KSRC) M=$(PWD) clean

else
include ../config.mk
KOBJ :=  $(KSRC)

VERSION_FILE := $(KOBJ)/include/linux/version.h
CONFIG_FILE  := $(KSRC)/include/linux/autoconf.h

# pick a compiler
CC := gcc
# standard flags for module builds
CFLAGS += -DLINUX -D__KERNEL__ -DMODULE -O2 -pipe -Wall
CFLAGS += -I$(KSRC)/include -I.
CFLAGS += $(shell [ -f $(KSRC)/include/linux/modversions.h ] && echo "-DMODVERSIONS -DEXPORT_SYMTAB -include $(KSRC)/include/linux/modversions.h")

CFLAGS += $(CFLAGS_EXTRA) -I../include
SMP := $(shell $(CC) $(CFLAGS) -E -dM $(CONFIG_FILE) | grep -w CONFIG_SMP | awk '{ print $$3 }')

ifneq ($(SMP),1)
  SMP := 0
endif
  
ifeq ($(SMP),1)
    CFLAGS += -D__SMP__
endif

.SILENT: $(TARGET)
$(TARGET): $(filter-out $(TARGET), $(CFILES:.c=.o))
	$(LD) $(LDFLAGS) -r $^ -o $@
	echo "** $(TARGET) built"

$(CFILES:.c=.o): Makefile

default:
	make

.PHONY: clean install

clean:
	rm -rf $(TARGET) $(TARGET:.ko=.o) $(TARGET:.ko=.mod.c) $(TARGET:.ko=.mod.o) $(CFILES:.c=.o) *~

endif
