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
#include "proc_handler.h"
#include "tcp_tune.h"

static struct state_machine* current_state_machine;

void register_state_machine(struct state_machine* sm)
{
    // TODO: free(current_state_machine);
    current_state_machine = sm;
}

static inline struct state_machine* create_default_state_machine(void)
{
    struct state_machine* default_state_machine =
                             kzalloc(sizeof(struct state_machine), GFP_KERNEL);
    // Creating actions
    struct state* initial_state = &default_state_machine->states[0];
    struct action* init_action = &initial_state->action;
    struct instruction* init_instruction = &init_action->instructions[0];

    struct state* cong_state = &default_state_machine->states[1];
    struct action* cong_action = &cong_state->action;
    struct instruction* cong_instruction = &cong_action->instructions[0];

    u32* c32 = get_address(CONSTANT_CONTEXT, 0);
    *c32 = 2;

    c32 = get_address(CONSTANT_CONTEXT + 1, 0);
    *c32 = 65535;

    init_action->instruction_count = 1;
    INSTRUCTION2(init_instruction, ASSIGN, CONSTANT_CONTEXT, CWND);

    cong_action->instruction_count = 2;
    INSTRUCTION3(cong_instruction, JGT, CWND, CONSTANT_CONTEXT + 1,
                 CONSTANT_CONTEXT);
    cong_instruction++;
    INSTRUCTION3(cong_instruction, MULTIPLY, CWND, CONSTANT_CONTEXT, CWND);


    // Creating transaction
    initial_state->transitions[ACKED].to = cong_state;
    cong_state->transitions[ACKED].to = cong_state;

    // Creating final action
    default_state_machine->final_action.instruction_count = 0;

    return default_state_machine;
}

static void tcp_tune_init(struct sock *sk)
{
    BUG_ON(!current_state_machine);
    pr_info("State machine registered: %p\n", current_state_machine);
    sk_register_state_machine(sk, current_state_machine);
}

static void tcp_tune_release(struct sock *sk)
{
}

static u32 tcp_tune_recalc_ssthresh(struct sock *sk)
{
    pr_info("ssthresh %u\n", get_value(u32, SSTHRESH, sk));
    return get_value(u32, SSTHRESH, sk);
}

#if TUNE_COMPAT < 19
static void tcp_tune_cong_avoid(struct sock *sk, u32 ack, u32 rtt,
                                u32 in_flight, int flag)
#else
static void tcp_tune_cong_avoid(struct sock *sk, u32 ack, u32 in_flight)
#endif
{
    struct tcp_tune* tcp_tune_ca = inet_csk_ca(sk);
    struct state_machine* current_state_machine = tcp_tune_ca->state_machine;
    BUG_ON(!current_state_machine);

    handle_event(current_state_machine, sk, ACKED);
}

static void tcp_tune_state(struct sock *sk, u8 new_state)
{
    if (new_state == TCP_CA_Loss) {
        struct tcp_tune* tcp_tune_ca = inet_csk_ca(sk);
        struct state_machine* current_state_machine = tcp_tune_ca->state_machine;
        BUG_ON(!current_state_machine);

        pr_info("DDEDDDDDRIOOOOOOPP\n");
        handle_event(current_state_machine, sk, DROPPED);
    }
}

static u32 tcp_tune_undo_cwnd(struct sock *sk) {
    return get_value(u32, CONSTANT_CONTEXT, sk);
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

#ifdef TCP_TUNE_DEBUG
    pr_info("%p\n", current_state_machine);
#endif

    if (!current_state_machine) {
        goto err;
    }

    error = register_sysctl_entries();
    if (error) {
        goto err;
    }

    error = register_tcptune_proc_fsops();
    if (error) {
        goto err2;
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

err2:
    unregister_tcptune_proc_fsops();

err:
    pr_info("TCP Tune cannot be registered\n");
    return error;
}
module_init(tcptune_module_init);

static __exit void tcptune_module_exit(void) {
    unregister_sysctl_enteries();
    unregister_tcptune_proc_fsops();
    tcp_unregister_congestion_control(&tcptune);
    pr_info("TCP Tune unregistered\n");
}
module_exit(tcptune_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Soheil Hassas Yeganeh <soheil@cs.toronto.edu>");
MODULE_DESCRIPTION("TCP parameter tuner");
MODULE_VERSION("0.1-ALPHA");

