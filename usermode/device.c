/*
 *  device.c - usermode interface to the vfs-undelete kernel module
 *  Copyright (C) 2007 Roman Mitnitski   <roman.mitnitski@gmail.com>
 *  Copyright (C) 2007 Mike Kemelmakher  <mike.kml@gmail.com>
 *
 *  This source code is licensed under the GNU General Public License,
 *  Version 2.  See the file COPYING for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <link_ioctl.h>

int get_device_fd()
{
    static int fd=-1;
    static int major=-1, minor=-1;
    if (fd < 0) {
	FILE * proc_device;
	proc_device = fopen("/proc/devices", "r");
	if (proc_device) {
	    char buf[80];
	    while (!feof(proc_device)) {
		char * n, * name;
		fgets(buf, 79, proc_device);
		n    = (char * ) strtok(buf," \n");
		name = (char * ) strtok(NULL, " \n");
		if (n && name) {
		    if (strcmp(name, "link")==0) {
			major = atoi(n);
			break;
		    }
		}
	    }
	    fclose(proc_device);
	}
	if (major > 0) {
	    unlink("/dev/vfs-undelete0");
	    if (mknod("/dev/vfs-undelete0", 0600 | S_IFCHR, makedev (major, 0)) != 0) {
		printf("Failed to create the device file\n");
		exit(0);
	    }
	    fd = open("/dev/vfs-undelete0", O_RDWR);
	    if (fd == -1) {
		printf("Failed to open the device file\n");
		exit(0);
	    }
	    return fd;
	} else {
	    printf("Please load the kernel part\n");
	    exit(0);
	}
    } else {
	return fd;
    }
}

int main(int argc, char * argv[])
{
    int fd, rc;
    struct link_what what;
    char * ep;
    unsigned long inum; 
    unsigned long pid;
    char * to;
    
    if (argc != 3 && argc != 4) {
      fprintf(stderr, "Usage: recover_file <inode> <pid> [<filename>]\n");
      exit (1);
    }
    inum = strtoul(argv[1], &ep, 10);
    pid  = strtoul(argv[2], &ep, 10);
    
    if (argc == 4) 
      to=argv [3];
    else 
      to="";
    
    what.link_to=to;
    what.link_to_len=strlen(to)+1;
    what.inode=inum;
    what.pid=pid;

    printf("Requesting recover of inode %lu for PID %d\n", what.inode, what.pid);
    
    fd = get_device_fd();   
    rc=ioctl(fd, LINK_IOCTL_CMD, &what);
    if (!rc) printf("Success\n");
    else perror("Something failed: ");
}

