# vfs-undelete
vfs-undelete - old (linux kernel 2.4) utility to undelete fies which are still open by manipulating VFS layer.

vfs-undelete is a small utility that lets the system administrator recover accidentally removed files as long as they are still open. Each "removed" file is restored consistently - even if it's under heavy I/O - by re-creating the link to the file's i-node in the filesystem.

The vfs-undelete project addresses a certain need that sometimes arises during system administration - specifically, the need for consistent recovery of database files lost during accidental removal. The specific user scenario that this project will address concerns accidental removal of database back end files while the database processes are still running.

As an example, consider SQL database files, stored on a file system. Suppose that the database is very active. If the storage files are removed by accident, the database will still run fine, because the files are not actually removed.

The files in use by the application remain in the file system, and will be cleaned up as soon as the application that uses them will shut down.

There are different existing methods to extract the data, such as copying the '/proc/PID/fd' entries, but all available methods fail to provide a consistent recovery that also assures that the application can continue to run and function well. (e.g. there's no way to prevent the application from changing the file while it's being copied)

Our vfs-undelete utility provides a robust way to recover lost files - by manipulating reference counts of inodes and linking the lost files back to the directories.

The utility is implemented as a kernel module and small usermode utility that controls this module. We beleive that this is a unique solution to this problem, and hope that it will serve the system adminstrators community.

