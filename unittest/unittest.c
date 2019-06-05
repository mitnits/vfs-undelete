/*
 *  device.c - usermode interface to the recover_link kernel module
 *  Copyright (C) 2007 Roman Mitnitski   <roman.mitnitski@gmail.com>
 *  Copyright (C) 2007 Mike Kemelmakher  <mike.kml@gmail.com>
 *
 *  This source code is licensed under the GNU General Public License,
 *  Version 2.  See the file COPYING for more details.
 */


#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


int main ()
{
   int fd;
   struct stat get_i;
   fd =  open ("/tmp/disappearing_file",O_RDWR|O_CREAT,0600);
   unlink("/tmp/disappearing_file");

   if (fstat(fd, &get_i) == 0) {
       printf("Look for inode %d for process %d\n", get_i.st_ino, getpid());
   } else {
       perror("");
   }
    
   pause ();

}
