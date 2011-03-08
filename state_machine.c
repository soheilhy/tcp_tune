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

struct state_machine* create_new_statemachine(void)
{
    struct state_machine* state_machine = 
                             kzalloc(sizeof(struct state_machine), GFP_KERNEL);
    return state_machine;
}
#define is_end(s)   (s[0] == 'E' && s[1] == 'N' && s[2] == 'D')
void add_transitions_to_state_machine(char* buffer, int start, int end, struct state_machine* state_machine) 
{

    int from_state_id;
    int to_state_id;
    struct state* from_state;
    struct state* to_state;

    buffer = buffer + start;
    while (*buffer != '\n') {
        buffer++;
    }

    while (!is_end(buffer)) { 
        buffer = skip_spaces(buffer);
    
        from_state_id = 0;
        while (!isspace(*buffer)) {
            from_state_id = from_state_id * 10 + (int) *buffer - '0';
            buffer++;
        }

        buffer = skip_spaces(buffer);

        to_state_id = 0;
        while (!isspace(*buffer)) {
            to_state_id = to_state_id * 10 + (int) *buffer - '0';
            buffer++;
        }
    
        from_state = &state_machine->states[from_state_id];
        to_state = &state_machine->states[to_state_id];

        buffer = skip_spaces(buffer);
        switch (*buffer) {
            case 'A':
                from_state->transitions[ACKED].to = to_state;
                break;
            case 'D':
                from_state->transitions[DROPPED].to = to_state;
                break;
            case 'T':
                from_state->transitions[TIMER].to = to_state;
                break;
        }

        pr_info("Connecting %d %d @ %d\n", from_state_id, to_state_id, *buffer);
        buffer = skip_spaces(buffer+1);
    }
}

void add_constants_to_state_machine(char* buffer, int start, int end, struct state_machine* state_machine)
{
    u32 constant_id;
    u32 value;
    u32* constant_register;

    buffer = buffer + start;
    while (*buffer != '\n') {
        buffer++;
    }

    while (!is_end(buffer)) { 
        buffer = skip_spaces(buffer);
    
        constant_id = 0;
        while (!isspace(*buffer)) {
            constant_id = constant_id * 10 + (int) *buffer - '0';
            buffer++;
        }

        buffer = skip_spaces(buffer);

        value = 0;
        while (!isspace(*buffer)) {
            value = value * 10 + (int) *buffer - '0';
            buffer++;
        }
    
        constant_register = get_address(CONSTANT_CONTEXT + constant_id, 0);
        *constant_register = value;

        buffer = skip_spaces(buffer + 1);
        pr_info("Constant %d <-- %d : %u \n", CONSTANT_CONTEXT + constant_id, *constant_register, get_value(u32, CONSTANT_CONTEXT, 0));

        
    }
}

void add_action_to_state_machine(char* buffer, int start, int end, struct state_machine* state_machine)
{
    u32 operand_code_1;
    u32 operand_code_2;
    u32 operand_code_3;

    int action_number = 0;
    struct action* action;

    buffer = skip_spaces(buffer + start);
    while (!isspace(*buffer)) {
        action_number = action_number * 10 + (int) *buffer - '0';
        buffer++;
    }

    action = &state_machine->states[action_number].action;

    while (!is_end(buffer)) { 
        int operand_count = 0;
        struct instruction* current_inst; 
        op_code_t op_code = ADD;

        buffer = skip_spaces(buffer);
   
        switch (*buffer) { 
            case '+':
                op_code = ADD;
                operand_count = 3;
                break;

            case '-':
                op_code = SUBTRACK;
                operand_count = 3;
                break;

            case '*':
                op_code = MULTIPLY;
                operand_count = 3;
                break;

            case '/':
                op_code = DIVIDE;
                operand_count = 3;
                break;

            case '%':
                op_code = MOD;
                operand_count = 3;
                break;

            case '=':
                op_code = JEQ;
                operand_count = 3;
                break;

            case '<':
                op_code = JLT;
                operand_count = 3;
                break;
                
            case '>':
                op_code = JGT;
                operand_count = 3;
                break;
            
            case 'a':
                op_code = ASSIGN;
                operand_count = 2;
                break;

            case '0':
                op_code = JZ;
                operand_count = 2;
                break;

            case 'n':
                op_code = JNZ;
                operand_count = 2;
                break;
        }

        buffer = skip_spaces(buffer + 1);

        operand_code_1 = 0;
        while (!isspace(*buffer)) {
            operand_code_1 = operand_code_1 * 10 + (int) *buffer - '0';
            buffer++;
        }

        buffer = skip_spaces(buffer);

        operand_code_2 = 0;
        while (!isspace(*buffer)) {
            operand_code_2 = operand_code_2 * 10 + (int) *buffer - '0';
            buffer++;
        }

        current_inst = action->instructions + action->instruction_count;

        if (operand_count > 2) {
            buffer = skip_spaces(buffer); 
            operand_code_3 = 0;
            while (!isspace(*buffer)) {
                operand_code_3 = operand_code_3 * 10 + (int) *buffer - '0';
                buffer++;
            }

            INSTRUCTION3(current_inst, 
                             op_code, operand_code_1, operand_code_2, operand_code_3); 
        } else { 
            INSTRUCTION2(current_inst, 
                             op_code, operand_code_1, operand_code_2); 
        }
        
        pr_info("action %d %d %d\n", op_code, operand_code_1, operand_code_2); 
        action->instruction_count++;
        buffer = skip_spaces(buffer+1);
    }
}



