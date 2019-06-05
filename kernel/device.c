/*
 *  device.c - usermode interface to the recover_link kernel module
 *  Copyright (C) 2007 Roman Mitnitski   <roman.mitnitski@gmail.com>
 *  Copyright (C) 2007 Mike Kemelmakher  <mike.kml@gmail.com>
 *  Copyright (C) 2009 Muli Ben-Yehuda   <muliby@gmail.com>
 *
 *  This source code is licensed under the GNU General Public License,
 *  Version 2.  See the file COPYING for more details.
 */

#include <linux/kernel.h>       /* printk() */
#include <linux/module.h>
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/mount.h>
#include <linux/file.h>
#include <linux/nsproxy.h>
#include <linux/pid.h>
#include <linux/dcache.h>
#include <linux/uaccess.h>
#include <linux/fdtable.h>
#include <asm/io.h>             /* virt_to_phys() */
#include "link_ioctl.h"

/*
 * The fops
 */
int link_ioctl   (struct inode *inode, struct file *file, u_int cmd, u_long arg);
int link_open    (struct inode *inode, struct file *filp);
int link_release (struct inode *inode, struct file *filp);

struct file_operations link_fops = {
llseek: NULL,
read:  NULL,
write: NULL,
ioctl: link_ioctl,
mmap:  NULL,
open:  link_open,
poll:  NULL,
release: link_release,
};

static int link_major = 0;
static int Device_Open = 0;

int link_open (struct inode *inode, struct file *filp)
{
	if (Device_Open)
		return -EBUSY;
	Device_Open++;

	return 0;
}

/*
 * Closing is even simpler.
 */
int link_release(struct inode *inode, struct file *filp)
{
	Device_Open--;

	return 0;
}

int link_recover(struct dentry * old_dentry, const char * newname);


/* need this for prepend_name (which is needed by vud_dentry_path */
static int prepend(char **buffer, int *buflen, const char *str, int namelen)
{
	*buflen -= namelen;
	if (*buflen < 0)
		return -ENAMETOOLONG;
	*buffer -= namelen;
	memcpy(*buffer, str, namelen);
	return 0;
}

/* need this for vud_dentry_path */
static int prepend_name(char **buffer, int *buflen, struct qstr *name)
{
	return prepend(buffer, buflen, name->name, name->len);
}

/*
 * Write full pathname from the root of the filesystem into the buffer.
 * Copied from dentry_path which is not exported to modules :-(
 */
char *vud_dentry_path(struct dentry *dentry, char *buf, int buflen)
{
	char *end = buf + buflen;
	char *retval;

	spin_lock(&dcache_lock);
	prepend(&end, &buflen, "\0", 1);
	if (d_unlinked(dentry) &&
		(prepend(&end, &buflen, "//deleted", 9) != 0))
			goto Elong;
	if (buflen < 1)
		goto Elong;
	/* Get '/' right */
	retval = end-1;
	*retval = '/';

	while (!IS_ROOT(dentry)) {
		struct dentry *parent = dentry->d_parent;

		prefetch(parent);
		if ((prepend_name(&end, &buflen, &dentry->d_name) != 0) ||
		    (prepend(&end, &buflen, "/", 1) != 0))
			goto Elong;

		retval = end;
		dentry = parent;
	}
	spin_unlock(&dcache_lock);
	return retval;
Elong:
	spin_unlock(&dcache_lock);
	return ERR_PTR(-ENAMETOOLONG);
}

/* Get the file name, given it's dentry */
char* recover_name(struct dentry * d, struct task_struct *p, char * buf, int size)
{
	char *f = NULL;

	printk(KERN_DEBUG "%s: got dentry %p, recovering name...\n",
	       __func__, d);

	f = vud_dentry_path(d, buf, size);
	if (!IS_ERR(f))
		printk(KERN_DEBUG "%s: dentry_path(%p) = '%s'\n",
		       __func__, d, f);
	else
		printk(KERN_DEBUG "%s: dentry_path error %d\n",
		       __func__, (int)f);
    
	if (!strcmp(f + strlen(f)-strlen("//deleted" ), "//deleted"))
		f[strlen(f)-strlen("//deleted")] = 0;
    
	return f;
}

/*
 * HACK: these are not exported to module :-(
 * Must be called under rcu_read_lock() or with tasklist_lock read-held.
 */
static inline struct task_struct *vud_find_task_by_pid_ns(pid_t nr, struct pid_namespace *ns)
{
	return pid_task(find_pid_ns(nr, ns), PIDTYPE_PID);
}

static inline struct task_struct *vud_find_task_by_vpid(pid_t vnr)
{
	return vud_find_task_by_pid_ns(vnr, current->nsproxy->pid_ns);
}

int link_ioctl(struct inode *inode, struct file *file, u_int cmd, u_long arg)
{
	struct link_what *what;
	struct task_struct *p;
	unsigned int fd, rc=0;

	struct files_struct * files;
	struct file * f=NULL;
	static char buf[PAGE_SIZE];
	char * recovered_name;

	/* TODO: copy from user properly */
	what=(struct link_what *)arg;

	/*
	 * TODO: locking - tasklist_lock is not exported to modules;
	 * is rcu_read_lock enough?
	 */
	rcu_read_lock();
	p = vud_find_task_by_vpid(what->pid);
	rcu_read_unlock();
   
	if (!p) {
		printk("No such PID %d\n", what->pid);
		return -ESRCH;
	}

	/* Equivalent of get_files_struct */
	task_lock(p);
	files = p->files;
	if (p->files)
		atomic_inc(&files->count);
	task_unlock(p);

	if (!files)
		return -EFAULT;

	spin_lock(&files->file_lock);  
	for (fd=0; fd < files->fdt->max_fds; fd++)
	{
		f= files->fdt->fd[fd];
		if (f != NULL) {
			if (f->f_dentry) {
				if (f->f_dentry->d_inode) {
					if (what->inode == f->f_dentry->d_inode->i_ino)
						break; 	
				}
			}
		}
	}
	spin_unlock(&files->file_lock);

 	if (!f)  {
		printk("Can't find that inode\n");
		/* need to undo get_files_struct() */
		return -ENOENT;
	}

	if (what->link_to_len >= ARRAY_SIZE(buf)) {
		printk("Control application gave us too long a name (%u)\n",
		       what->link_to_len);
		/* cleanup missing here */
		return -E2BIG;
	}
		
	if (copy_from_user(buf, what->link_to, what->link_to_len)) {
		printk("Failed to copy file name from the control application");
		return -EFAULT;
	}
	buf[ARRAY_SIZE(buf) - 1] = '\0';

	if (buf[0] == 0) {
		recovered_name = recover_name(f->f_dentry, p, buf, PAGE_SIZE);
	} else {
		recovered_name = buf;
	}

	rc = link_recover(f->f_dentry, recovered_name);
	return rc;   
}

/*
 * Module housekeeping.
 */
static int link_init(void)
{
	int result;
    
	result = register_chrdev(0, "link", &link_fops);
	if (result < 0)
	{
		printk(KERN_WARNING "simple: unable to get major %d\n", link_major);
		return result;
	}
	if (link_major == 0)
		link_major = result;

	return 0;
}


static void link_cleanup(void)
{
	unregister_chrdev(link_major, "link");
}

module_init(link_init);
module_exit(link_cleanup);

MODULE_LICENSE("GPL");
