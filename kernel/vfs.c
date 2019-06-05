/*
 *  device.c - usermode interface to the recover_link kernel module
 *  Copyright (C) 2007 Roman Mitnitski   <roman.mitnitski@gmail.com>
 *  Copyright (C) 2007 Mike Kemelmakher  <mike.kml@gmail.com>
 *
 *  This source code is licensed under the GNU General Public License,
 *  Version 2.  See the file COPYING for more details.
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/quotaops.h>
#include <linux/pagemap.h>
#include <linux/dnotify.h>
#include <linux/smp_lock.h>
#include <linux/personality.h>
#include <linux/version.h>

#include <linux/namei.h>
#include <asm/uaccess.h>

int link_recover(struct dentry * old_dentry, const char * to)
{
	int error;
	struct dentry *new_dentry;
	struct nameidata nd;

	error = path_lookup(to, LOOKUP_PARENT, &nd);
	if (error)
		goto exit;

	error = -EXDEV;
	new_dentry = lookup_create(&nd, 0);
	error = PTR_ERR(new_dentry);

	if (IS_ERR(new_dentry))
	    goto exit;

	old_dentry->d_inode->i_nlink = 1;

	/* Must check that old_dentry and nd.dentry are on the same FS  */
	error = vfs_link(old_dentry, nd.path.dentry->d_inode, new_dentry);

	mutex_unlock(&nd.path.dentry->d_inode->i_mutex);
	dput(new_dentry);

	path_put(&nd.path);

exit:
	return error;
}

