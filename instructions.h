/*
 * TCP tune action
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

#ifndef TUNE_ACTION_H
#define TUNE_ACTION_H

#include <linux/socket.h>
#include <linux/tcp.h>
#include <net/tcp.h>

#include "version.h"

#define MAX_OPERANDS    3
#define MAX_GLOBAL_REGISTERS 2
#define MAX_CONSTANTS 16
#define MAX_TIMERS 2

#define op_code_t       u8
#define value_code_t    u8

#define STAT_CONTEXT        0
#define GLOBAL_CONTEXT      (1 << 6)
#define FLOW_CONTEXT        (1 << 7)
#define CONSTANT_CONTEXT    (3 << 6)

enum OP_CODES {
    ADD = 0,
    SUBTRACK,
    MULTIPLY,
    DIVIDE,
    MOD, 
    ASSIGN,

    GLOCK = 16,
    GUNLOCK, 

    JNZ = 32,
    JZ,
    JLT,
    JGT,
    JEQ,

    TIMER_REG = 128
};

enum VALUE_CODES {
    // Statistical values
    SRTT = STAT_CONTEXT,
    RTO,
    // Global values
    R0 = GLOBAL_CONTEXT,
    R1,
    R2,
    R3,
    // Flow values
    CWND = FLOW_CONTEXT,
    INIT_CWND,
    CWND_CLAMP,
    SSTHRESH,
};


struct locals {
    u32 init_cwnd;
    u32 cwnd_clamp;
};


struct timers {
    struct timer_list timers[MAX_TIMERS];
};



struct instruction {
    op_code_t op_code;
    value_code_t operands[MAX_OPERANDS];
};


#define INSTRUCTION2(inst, code, op1, op2) \
            { \
                inst->op_code = code; \
                inst->operands[0] = op1; \
                inst->operands[1] = op2; \
            }


#define INSTRUCTION3(inst, code, op1, op2, op3) \
            { \
                inst->op_code = code; \
                inst->operands[0] = op1; \
                inst->operands[1] = op2; \
                inst->operands[2] = op3; \
            }

#define MAX_INSTRUCTIONS    20

struct action {
    u8 instruction_count;
    struct instruction instructions[MAX_INSTRUCTIONS];
};

void* get_address(value_code_t value_code, struct sock* sk); 

#define get_value(type, code, sk)    *((type*)get_address(code, sk))

u32 execute_opcode(op_code_t op_code, 
                    value_code_t value_code1, 
                    value_code_t value_code2, 
                    value_code_t value_code3,
                    struct sock* sk);

#define execute_instruction(instruction, sk) \
        execute_opcode( (instruction)->op_code, \
                        (instruction)->operands[0], \
                        (instruction)->operands[1], \
                        (instruction)->operands[2], \
                        sk)

void execute_action(struct action* action, struct sock* sk);
#endif
