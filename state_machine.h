#ifndef TUNE_STATE_MACHINE
#define TUNE_STATE_MACHINE

#include "instructions.h"

#define Condition   u8
#define ACKED       0x01
#define DROPPED     0x02
#define TIMER_1     0x04
#define TIMER_2     0x08

struct state;

struct transition {
    struct state* to;
    condition condition;
}

struct state {
    struct action*      action;
    struct transition   transitions[];
}

struct state_machine {
    struct state    states[];
    struct action*  final_action;
}

enum event {
    INIT_EVENT,
    ACKED,
    DROPPED,
    TIMER
}

static int register_new_state_machine(struct state_machine* state_machine);

static int sk_register_state_machine(struct tcp_sock* sk, struct state_machine* state_machine);

static struct state* handle_event(struct state_machine* state_machine, struct sock* sk, enum event e);

#endif
