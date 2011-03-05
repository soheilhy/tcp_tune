/*
 * tcp tune proc handler
 *  
 * Copyright (C) 2010, Soheil Hassas Yeganeh <soheil@cs.toronto.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef TUNE_PROC_HANDLER
#define TUNE_PROC_HANDLER

#include <linux/proc_fs.h>

static int tcpflowspy_open(struct inode * inode, struct file * file) {
    return 0;
}

static ssize_t tcpflowspy_read(struct file *file, char __user *buf,
        size_t len, loff_t *ppos) {

}


static ssize_t
    procfs_write(struct file *file, const char *buffer, size_t len, loff_t * off)
{
    if ( len > PROCFS_MAX_SIZE ) {
        procfs_buffer_size = PROCFS_MAX_SIZE;
    } else {
        procfs_buffer_size = len;
    }

    if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
        return -EFAULT;
    }

    printk(KERN_INFO "procfs_write: write %lu bytes\n", procfs_buffer_size);

    return procfs_buffer_size;
}


static const struct file_operations tcpflowspy_fops = {
    .owner	 = THIS_MODULE,
    .open	 = tcp_tune_proc_open,
    .read    = tcp_tune_proc_read,
    .write   = tcp_tune_proc_write,
};

#endif
