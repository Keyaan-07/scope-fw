#ifndef FRONTEND_CONTROL
#define FRONTEND_CONTROL
#include "pico/stdlib.h"

void pga_set_gain(int multiplier);
void tmux_set_coupling(bool state);
void init_frontend(void);

#endif