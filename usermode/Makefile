all:    link_ctl


link_ctl: device.c
	gcc -static -o vfs-undelete -I../include device.c

clean:	
	rm -f link_ctl *~
