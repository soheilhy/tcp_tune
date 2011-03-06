/*
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

#ifndef TCP_TUNE_H
#define TCP_TUNE_H

#include <linux/kprobes.h>
#include <linux/socket.h>
#include <linux/tcp.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/timer.h>
#include <net/tcp.h>


#include "state_machine.h"
/*
 * Turn it on for debug information.
 */
//#define TCP_TUNE_DEBUG


#define GLOBAL_REGISTER_COUNT 2

static struct {
    u32 r32[GLOBAL_REGISTER_COUNT];
    u64 r64[GLOBAL_REGISTER_COUNT];
} globals;


struct locals {
    u32 init_cwnd;
    u32 cwnd_clamp;
};

#define MAX_CONSTANTS 16

static struct {
    u32 c32[MAX_CONSTANTS];   
} constants;

#define MAX_TIMERS 2

struct timers {
    struct timer_list timers[MAX_TIMERS];
};

static struct timer_list timer1; 

struct tcp_tune {
    struct state_machine* state_machine;
    struct state* current_state;

    struct locals* locals;
    struct timers* timers;
};

#endif 
