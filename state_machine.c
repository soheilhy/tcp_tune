#include "state_machine.h"

static int register_new_state_machine(struct state_machine* state_machine) {
    return -1;
}

static int sk_register_state_machine(struct tcp_sock* sk, struct state_machine* state_machine) {
    struct tcp_sock* tcp_socket = tcp_sk(sk);
    struct inet_connection_sock* inet_sock = inet_sk(sk);
    struct tcp_tune* tcp_tune_ca = inet_csk_ca(sk);

    tcp_tune_ca->state_machine = state_machine;
    tcp_tune_ca->current_state = 0;

    tcp_tune_ca->current_state = handle_event(state_machine, sk, INIT_EVENT);
    
}

static struct state* handle_event(struct state_machine* state_machine, struct sock* sk, enum event e) {     
    switch (e) { 
        case INIT_EVENT:
            break;
        case ACKED:
            break;
        case DROPPED:
            break;
        case TIMER:
            break;
    }
}


