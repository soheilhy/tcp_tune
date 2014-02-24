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

#ifndef TUNE_PROC_HANDLER_H
#define TUNE_PROC_HANDLER_H

#include <linux/proc_fs.h>

int tcptune_proc_open(struct inode * inode, struct file * file);
ssize_t tcptune_proc_read(struct file *file, char __user *buf, size_t len,
                          loff_t *ppos);
ssize_t tcptune_proc_write(struct file *file, const char *user_buffer,
                           size_t len, loff_t *off);
int register_tcptune_proc_fsops(void);
void unregister_tcptune_proc_fsops(void);
#endif

