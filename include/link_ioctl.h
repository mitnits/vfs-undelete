/*
 *  device.c - usermode interface to the recover_link kernel module
 *  Copyright (C) 2007 Roman Mitnitski   <roman.mitnitski@gmail.com>
 *  Copyright (C) 2007 Mike Kemelmakher  <mike.kml@gmail.com>
 *
 *  This source code is licensed under the GNU General Public License,
 *  Version 2.  See the file COPYING for more details.
 */


#ifndef LINK_IOCTL_H
#define LINK_IOCTL_H

#define LINK_IOCTL_CMD 10

struct link_what {
    unsigned long  inode;
    char * link_to;
    unsigned int link_to_len;
    int pid;
};

#endif
