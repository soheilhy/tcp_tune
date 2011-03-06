#ifndef TUNE_STATE_MACHINE
#define TUNE_STATE_MACHINE

#include "version.h"
#include "instructions.h"

#define MAX_EVENT   4

enum event {
    ACKED,
    DROPPED,
    TIMER,
    INIT_EVENT = MAX_EVENT - 1,
};

struct transition {
    struct state* to;
};

struct state {
    struct action action;
    struct transition transitions[MAX_EVENT];
};

#define MAX_STATES  20

struct state_machine {
    struct state states[MAX_STATES];
    struct action final_action;
};

int sk_register_state_machine(struct sock* sk, struct state_machine* state_machine);
struct state* handle_event(struct state_machine* state_machine, struct sock* sk, enum event e);

#endif
