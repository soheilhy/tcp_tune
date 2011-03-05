#include "instructions.h"

void* get_address(value_code_t value_code, struct sock* sk)
{
    struct tcp_sock* tcp_socket = tcp_sk(sk);
    struct inet_connection_sock* inet_sock = inet_sk(sk);
    struct tcp_tune* tcp_tune_ca = inet_csk_ca(sk);

    if (likely(value_code < CONSTANT_CONTEXT)) {
        switch(value_code) {
            case SRTT:
                return &tcp_socket->srtt;
            case RTO:
                return &inet_sk.icsk_rto;

            case CWND:
                return &tcp_socket->snd_cwnd;
            case INIT_CWND:
                return &tcp_tune_ca->locals->init_cwnd; 
            case CWND_CLAMP:
                return &tcp_tune_ca->locals->cwnd_clamp;

            case R0:
                return &globals.r32[0];  
            case R1:
                return &globals.r32[1];
            case R2:
                return &globals.r64[0];
            case R3:
                return &globals.r64[1];
        }
    } else {
       return &constants.c32[value_code - CONSTANT_CONTEXT];
    }
}

int execute_opcode(op_code_t op_code, 
                    value_code_t value_code1, 
                    value_code_t value_code2, 
                    value_code_t value_code3,
                    struct sock* sk) {
    switch (op_code) {
        case ADD:
            u32* res = (u32*) get_address(value_code3, sk);
            *res = (u32) (*get_address(value_code1, sk)) + (u32) (*get_address(value_code2, sk));
            break;

        case SUBTRACK:
            u32* res = (u32*) get_address(value_code3, sk);
            *res = (u32) (*get_address(value_code1, sk)) - (u32) (*get_address(value_code2, sk));
            break;
            
        case MULTIPLY:
            u32* res = (u32*) get_address(value_code3, sk);
            *res = (u32) (*get_address(value_code1, sk)) * (u32) (*get_address(value_code2, sk));
            break;

        case DIVIDE:
            u32* res = (u32*) get_address(value_code3, sk);
            *res = (u32) (*get_address(value_code1, sk)) / (u32) (*get_address(value_code2, sk));
            break;

        case MOD:
            u32* res = (u32*) get_address(value_code3, sk);
            *res = (u32) (*get_address(value_code1, sk)) % (u32) (*get_address(value_code2, sk));
            break;

        case ASSIGN:
            void* res = get_address(value_code2, sk);
            *res = (*get_address(value_code1, sk));
            break;

        case LOCK:
            break;

        case UNLOCK:
            break;

        case TIMER: 
            break;
        
        case JNZ:
            break;

        case JZ:
            break;

        case JLT:
            break;

        case JGT:
            break;
    }


    return 1;
}

static void execute_action(struct action* action, struct sock* sk) {
    int i = 0;

    if (unlikely(!action)) {
        return;
    }

    for (i = 0; i < action->instruction_count; ) {
        i += execute_instruction(action->instructions[i], sk); 
    }
}

