#ifndef kdp_server_h
#define kdp_server_h

#include "arm64_state.h"

int start_kdp_server(void);
void kdp_handle_stop(arm_context_t* state);

#endif
