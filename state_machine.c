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

#include "state_machine.h"
#include "tcp_tune.h"

int sk_register_state_machine(struct sock* sk, struct state_machine* state_machine) {
    //struct tcp_sock* tcp_socket = tcp_sk(sk);
    //struct inet_connection_sock* inet_sock = inet_sk(sk);
    struct tcp_tune* tcp_tune_ca = (struct tcp_tune*) inet_csk_ca(sk);

    tcp_tune_ca->state_machine = state_machine;
    tcp_tune_ca->current_state = 0;

    handle_event(state_machine, sk, INIT_EVENT);
    
    return 0;
}

#define run_state_action(state, sk) execute_action(&state->action, sk)

struct state* handle_event(struct state_machine* state_machine, struct sock* sk, enum event e) {     

    //struct tcp_sock* tcp_socket = tcp_sk(sk);
    //struct inet_sock* inet_sock = inet_sk(sk);
    struct tcp_tune* tcp_tune_ca = inet_csk_ca(sk);
    struct state* current_state = tcp_tune_ca->current_state;
    struct transition* t;

    switch (e) { 
        case INIT_EVENT:
            pr_info("init is called\n");
            current_state = &state_machine->states[0];
            run_state_action(current_state, sk);
            break;

        case ACKED:
        case DROPPED:
        case TIMER:
            t = &current_state->transitions[e];

            if (unlikely(!t)) {
                return 0;
            }

            current_state = t->to;
            run_state_action(current_state, sk);
            break;
    }

    tcp_tune_ca->current_state = current_state;
    return current_state;
}


