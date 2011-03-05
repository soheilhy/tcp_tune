#ifndef TCP_TUNE_H
#define TCP_TUNE_H

#define GLOBAL_REGISTER_COUNT = 2

static struct {
    u32 r32[GLOBAL_REGISTER_COUNT];
    u64 r64[GLOBAL_REGISTER_COUNT];
} globals;


static struct locals {
    u32 init_cwnd;
    u32 cwnd_clamp;
};

#define MAX_CONSTANTS 16

static struct {
    u32 c32[MAX_CONSTANTS];   
} constants;

#define MAX_TIMERS 2

static struct timers {
    struct timer_list timers[MAX_TIMERS];
};

struct timer_list timer1; 

struct tcp_tune {
    struct state_machine* state_machine;
    struct state* current_state;

    struct locals* locals;
    struct timers* timers;

};

#endif 
