include config.mk

all: kernel usermode unittest
	(cd kernel; make )
	(cd usermode; make)
	(cd unittest; make)	

clean: kernel usermode
	(cd kernel; make clean)
	(cd usermode; make clean)
	(cd unittest; make clean)
