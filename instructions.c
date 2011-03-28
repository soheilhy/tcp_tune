#include "instructions.h"
#include "tcp_tune.h"


struct {
    u32 r32[MAX_GLOBAL_REGISTERS];
    u64 r64[MAX_GLOBAL_REGISTERS];
} globals;


struct {
    u32 c32[MAX_CONSTANTS];   
} constants;

//static struct timer_list timer1; 

void* get_address(value_code_t value_code, struct sock* sk)
{
    struct tcp_sock* tcp_socket = tcp_sk(sk);
    struct inet_connection_sock* inet_c_sk = inet_csk(sk);
    struct tcp_tune* tcp_tune_ca = inet_csk_ca(sk);

    if (likely(value_code < CONSTANT_CONTEXT)) {
        switch(value_code) {
            case SRTT:
                return &tcp_socket->srtt;
            case RTO:
                return &inet_c_sk->icsk_rto;

            case CWND:
                return &tcp_socket->snd_cwnd;
            case INIT_CWND:
                return &tcp_tune_ca->locals->init_cwnd; 
            case CWND_CLAMP:
                return &tcp_tune_ca->locals->cwnd_clamp;
            case SSTHRESH:
                return &tcp_socket->snd_ssthresh;

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

    return 0;
}


u32 execute_opcode(op_code_t op_code, 
                    value_code_t value_code1, 
                    value_code_t value_code2, 
                    value_code_t value_code3,
                    struct sock* sk) {
    u32* res;
    u32 value;
    u32 value2;

    switch (op_code) {
        case ADD:
            res = (u32*) get_address(value_code3, sk);
            *res = get_value(u32, value_code1, sk) + get_value(u32, value_code2, sk);
            break;

        case SUBTRACK:
            res = (u32*) get_address(value_code3, sk);
            *res = get_value(u32, value_code1, sk) - get_value(u32, value_code2, sk);
            break;
            
        case MULTIPLY:
            res = (u32*) get_address(value_code3, sk);
            *res = get_value(u32, value_code1, sk) * get_value(u32, value_code2, sk);

#ifdef TCP_TUNE_DEBUG
            pr_info("Multiplying %u %u %u %u %u \n", 
                    get_value(u32, value_code1, sk),
                    value_code2, 
                    get_value(u32, value_code2, sk), 
                    constants.c32[0],
                    *res);
#endif
 
            break;

        case DIVIDE:
            res = (u32*) get_address(value_code3, sk);
            *res = get_value(u32, value_code1, sk) / get_value(u32, value_code2, sk);
            break;

        case MOD:
            res = (u32*) get_address(value_code3, sk);
            *res = get_value(u32, value_code1, sk) % get_value(u32, value_code2, sk);
            break;

        case ASSIGN:
            res = get_address(value_code2, sk);
            *res = get_value(u32, value_code1, sk);

            pr_info("ASSIGN %u %u %u %u %u \n", 
                    get_value(u32, value_code1, sk),
                    value_code2, 
                    get_value(u32, value_code2, sk), 
                    constants.c32[0],
                    get_value(u32, CONSTANT_CONTEXT, 0));

            break;

        case GLOCK:
            break;

        case GUNLOCK:
            break;

        case TIMER_REG: 
            break;
        
        case JNZ:
            value = get_value(u32, value_code1, sk);
            if (value) {
                return get_value(u32, value_code2, sk);
            }
            break;

        case JZ:
            value = get_value(u32, value_code1, sk);
            if (!value) {
                return get_value(u32, value_code2, sk);
            }
            break;

        case JLT:
            value = get_value(u32, value_code1, sk);
            value2 = get_value(u32, value_code2, sk);
            if (value < value2) {
                return get_value(u32, value_code2, sk);
            }
            break;

        case JGT:
            value = get_value(u32, value_code1, sk);
            value2 = get_value(u32, value_code2, sk);
            if (value > value2) {
                return get_value(u32, value_code2, sk);
            }
            break;

        case JEQ:
            value = get_value(u32, value_code1, sk);
            value2 = get_value(u32, value_code2, sk);
            if (value == value2) {
                return get_value(u32, value_code2, sk);
            }
            break;
    }


    return 1;
}

void execute_action(struct action* action, struct sock* sk) {
    int i = 0;

    if (unlikely(!action)) {
        return;
    }

    for (i = 0; i < action->instruction_count; ) {
        i += execute_instruction(&(action->instructions[i]), sk); 
    }
    pr_info("CWND %u\n", tcp_sk(sk)->snd_cwnd);
}

