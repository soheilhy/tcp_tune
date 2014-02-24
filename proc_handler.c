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

#include <linux/string.h>

#include "syntax.h"
#include "proc_handler.h"
#include "state_machine.h"
#include "tcp_tune.h"

enum def_type {
    TRANSITION_T,
    CONSTANT_T,
    FINAL_ACTION_T,
    ACTION_T
};

struct state_machine* new_sm;
int tcptune_proc_open(struct inode * inode, struct file * file)
{
    new_sm = create_new_statemachine();
    return 0;
}

ssize_t tcptune_proc_read(struct file *file, char __user *buf, size_t len,
                          loff_t *ppos) {
    return 0;
}


#define MAX_PROC_BUF_SIZE   2048
static char buffer[MAX_PROC_BUF_SIZE];


static int str_contains(const char* token, int start_pos, size_t size)
{
    const char* token_p = token;
    char* text_p = buffer + start_pos;
    size_t temp_len,
           len;
    len = temp_len = strlen(token);


    while (size) {

        if (likely(*token_p != *text_p)) {
            token_p = token;
            temp_len = len;
        } else {
            token_p++;
            temp_len--;

            if (!temp_len) {
                return start_pos - len + 1;
            }
        }

        text_p++;
        size--;
        start_pos++;
    }

    return -1;
}

static int end_of_def_block(int start_pos, size_t size)
{
    int index = str_contains(END_TOKEN, start_pos, size);
    if ( likely(index >= 0) ) {
        return index;
    } else {
        return -1;
    }
}

static int begin_of_def_block(int start_pos, size_t size)
{
    int index = str_contains(BEGIN_TOKEN, start_pos, size);
    if ( likely(index >= 0) ) {
        return index;
    } else {
        return -1;
    }
}

static enum def_type get_def_type(int begin_index, int end_index) {
    char* id = skip_spaces(buffer + begin_index);
    pr_info("TYPE %c\n", *id);
    switch (*id) {
        case 'T':
            return TRANSITION_T;
            break;
        case 'A':
            return FINAL_ACTION_T;
            break;
        case 'C':
            return CONSTANT_T;
            break;
        default:
            return ACTION_T;
    }
}

static int parse_block(int start_pos, size_t size,
                       struct state_machine *state_machine) {
    int begin_index = begin_of_def_block(start_pos, size);
    pr_info("block begin %d\n", begin_index);
    if ( likely(begin_index >= 0) ) {
        int end_index;
        enum def_type type;
        begin_index += strlen(BEGIN_TOKEN) + 1;
        end_index = end_of_def_block(begin_index, size) - 1;
        type = get_def_type(begin_index, end_index);

        switch (type) {
            case TRANSITION_T:
                add_transitions_to_state_machine(buffer, begin_index, end_index,
                                                 state_machine);
                break;

            case CONSTANT_T:
                add_constants_to_state_machine(buffer, begin_index, end_index,
                                               state_machine);
                break;

            case FINAL_ACTION_T:
            case ACTION_T:
                add_action_to_state_machine(buffer, begin_index, end_index,
                                            state_machine);
                break;
        }

        return end_index + strlen(END_TOKEN) + 1;

    } else {
        return -1;
    }
}

ssize_t tcptune_proc_write(struct file *file, const char *user_buffer,
                           size_t len, loff_t *off) {
    size_t read_length = min_t(size_t, MAX_PROC_BUF_SIZE, len);
    int index = 0,
        index_ret = 0;

    if ( copy_from_user(buffer, user_buffer, read_length) ) {
        return -EFAULT;
    }

    printk(KERN_INFO "procfs_write: write %lu bytes\n", read_length);

    while ((index = parse_block(index, read_length, new_sm)) != -1) {
        index_ret = index;
    }

    register_state_machine(new_sm);
    pr_info("Constant reg: %u\n", get_value(u32, CONSTANT_CONTEXT, 0));

    return index_ret;
}

static const struct file_operations tcptune_proc_fops = {
    .owner	 = THIS_MODULE,
    .open	 = tcptune_proc_open,
    .read    = tcptune_proc_read,
    .write   = tcptune_proc_write,
};

static const char procname[] = "tcptune";

int register_tcptune_proc_fsops(void)
{
    return proc_net_fops_create(
#if TUNE_COMPAT >= 32
                &init_net,
#endif
                procname, S_IRWXU | S_IRGRP | S_IROTH ,
                &tcptune_proc_fops) == 0;
}

void unregister_tcptune_proc_fsops(void)
{
    proc_net_remove(
#if TUNE_COMPAT >= 32
            &init_net,
#endif
            procname);

}

