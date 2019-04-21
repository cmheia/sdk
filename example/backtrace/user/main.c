#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unwind.h>

typedef struct stack_trace_state {
    const void **addr;
    int          count;
} stack_trace_state;

_Unwind_Reason_Code trace_fn(_Unwind_Context *context, void *arg)
{
    stack_trace_state *state = (stack_trace_state *) arg;
    if (state->count > 0) {
        void *ip = (void *) _Unwind_GetIP(context);
        if (ip) {
            state->addr[0] = ip;
            state->count--;
            state->addr++;
        }
    }
    return _URC_NO_REASON;
}

#define __GET_BACKTRACE(max_depth)                                            \
    do {                                                                      \
        const void *      stack[max_depth];                                   \
        stack_trace_state state;                                              \
        int               i;                                                  \
        memset(stack, 0, sizeof(stack));                                      \
        state.addr               = stack;                                     \
        state.count              = max_depth;                                 \
        _Unwind_Reason_Code code = _Unwind_Backtrace(trace_fn, &state);       \
        printf("Stack trace count: %d, code: %d\n",                           \
               max_depth - state.count,                                       \
               code);                                                         \
        for (i = 0; i < max_depth - state.count; i++) {                       \
            printf("%p ", stack[i]);                                          \
        }                                                                     \
        printf("\n-----\n");                                                  \
    } while (0)

void backtrace_fn(void)
{
#define MAX_DEPTH 20
    const void *      stack[MAX_DEPTH];
    stack_trace_state state;
    state.addr  = stack;
    state.count = MAX_DEPTH;
    int i;

    memset(stack, 0, sizeof(stack));
    _Unwind_Reason_Code code = _Unwind_Backtrace(trace_fn, &state);
    printf("Back trace count: %d, code: %d\n", MAX_DEPTH - state.count, code);
    for (i = 0; i < MAX_DEPTH - state.count; i++) {
        printf("%p ", stack[i]);
    }
    printf("\n-----\n");
}

void callee_fn(void)
{
    __GET_BACKTRACE(10);
}

void caller_fn(void)
{
    backtrace_fn();
    callee_fn();
}

void UserMain(void)
{
    printf("backtrace " __DATE__ " " __TIME__ "\r\n");
    __GET_BACKTRACE(10);
    caller_fn();
}
