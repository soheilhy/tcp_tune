#ifndef TUNE_STATE_MACHINE_H
#define TUNE_STATE_MACHINE_H

#include <linux/ctype.h>
#include "version.h"
#include "instructions.h"

#define MAX_EVENT   4
#define MAX_TRANS   4

enum event {
    ACKED,
    DROPPED,
    TIMER,
    INIT_EVENT = MAX_EVENT - 1,
};

struct condition {
    enum conditional_opcode;
    int lhs;
    int rhs;
    struct state* to;
};

struct transition {
    struct condition conditions[MAX_TRANS];
//    struct state* to;
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

int sk_register_state_machine(struct sock *sk,
                              struct state_machine *state_machine);
struct state *handle_event(struct state_machine *state_machine, struct sock *sk,
                           enum event e);

struct state_machine* create_new_statemachine(void);

// Parsing functionalities
void add_transitions_to_state_machine(char *buffer, int start, int end,
                                      struct state_machine *state_machine);
void add_constants_to_state_machine(char *buffer, int start, int end,
                                    struct state_machine *state_machine);
void add_action_to_state_machine(char *buffer, int start, int end,
                                 struct state_machine *state_machine);

#endif

