/*
 * tcptune - Tune TCP global parameters 
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
#include "sysctl_handler.h"
#include "tcp_tune.h"


MODULE_AUTHOR("Soheil Hassas Yeganeh <soheil@cs.toronto.edu>");
MODULE_DESCRIPTION("TCP parameter tuner");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1-ALPHA");

static struct state_machine* current_state_machine; 

static inline struct state_machine* create_default_state_machine(void) 
{
    struct state_machine* default_state_machine = 
                                kzalloc(sizeof(struct state_machine), GFP_KERNEL);
    // Creating actions
    struct state* initial_state = &default_state_machine->states[0];
    struct action* init_action = &initial_state->action;
    struct instruction* init_instruction = &initial_action->instructions[0];

    struct state* cong_state = &default_state_machine->states[1];
    struct action* cong_action = &cong_state->action;
    struct instruction* cong_instruction = &cong_action->instructions[0];

    constants.c32[0] = 2U;

    initial_action->instruction_count = 1;
    INSTRUCTION2(init_instruction, ASSIGN, CONSTANT_CONTEXT, CWND);

    cong_action->instruction_count = 1;
    INSTRUCTION3(cong_instruction, MULTIPLY, CWND, CONSTANT_CONTEXT, CWND);
    
    // Creating transaction
    initial_state->transactions[ACKED].to = cong_instruction;  
    cong_state->transactions[ACKED].to = cong_instruction;  

    // Creating final action    
    default_state_machine->final_action.instruction_count = 0; 
}

static void tcp_tune_init(struct sock *sk) 
{
    BUG_ON(!current_state_machine);
    sk_register_state_machine(sk, current_state_machine);
}

static void tcp_tune_release(struct sock *sk) 
{
}

static u32 tcp_tune_recalc_ssthresh(struct sock *sk) 
{
    struct tcp_tune* tcp_tune_ca = inet_csk_ca(sk);
    struct state_machine* current_state_machine = tcp_tune_ca->state_machine;
    BUG_ON(!current_state_machine);

    handle_event(current_state_machine, sk, ACKED);
}

#if TUNE_COMPAT < 19
static void tcp_tune_cong_avoid(struct sock *sk, u32 ack, u32 rtt, u32 in_flight, int flag)
#else
static void tcp_tune_cong_avoid(struct sock *sk, u32 ack, u32 in_flight)
#endif
{
}

static void tcp_tune_state(struct sock *sk, u8 new_state) {
}

static u32 tcp_tune_undo_cwnd(struct sock *sk) {
}
#if TUNE_COMPAT < 19
static void tcp_tune_acked(struct sock *sk, u32 cnt)
#else
static void tcp_tune_acked(struct sock *sk, u32 cnt, s32 rtt_us)
#endif
{
}

static struct tcp_congestion_ops tcptune = {
	.init		= tcp_tune_init,
    .release    = tcp_tune_release,
	.ssthresh	= tcp_tune_recalc_ssthresh,
	.cong_avoid	= tcp_tune_cong_avoid,
	.set_state	= tcp_tune_state,
	.undo_cwnd	= tcp_tune_undo_cwnd,
	.pkts_acked     = tcp_tune_acked,
	.owner		= THIS_MODULE,
	.name		= "tune",
};

static __init int tcptune_module_init(void) {
    int error = 0;
    
    BUILD_BUG_ON(sizeof(struct tcp_tune) > ICSK_CA_PRIV_SIZE);

    current_state_machine = create_default_state_machine();
    if (!current_state_machine) {
        goto err;
    }

    error = register_sysctl_entries();
    if (error) {
        goto err;
    }

#if TUNE_COMPAT > 18
    tcptune.flags |= TCP_CONG_NON_RESTRICTED;
#endif
    error = tcp_register_congestion_control(&tcptune);
    if (error) {
        goto err; 
    }

    pr_info("TCP Tune registeration finalized \n"); 
    return 0;

err:
    pr_info("TCP Tune cannot be registered\n");
    return error;
}
module_init(tcptune_module_init);

static __exit void tcptune_module_exit(void) {
    unregister_sysctl_enteries();
    tcp_unregister_congestion_control(&tcptune);
    pr_info("TCP Tune unregistered\n");
}
module_exit(tcptune_module_exit);

