#include "frontend_control.h"
#include <stdio.h>

//add proper pinouts here
#define G0 1
#define G1 1
#define G2 1
#define G3 1
#define G4 1
#define ACDC_SEL 1

// taken from the PGA281 datasheet, page 6
void pga_set_gain(int multiplier){
    switch(multiplier){
        case 1:
            printf("Setting Gain to 1x");
            gpio_put(G0, 1);
            gpio_put(G1, 1);
            gpio_put(G2, 0);
            gpio_put(G3, 0);
            gpio_put(G4, 0);
            break;
        case 8:
            printf("Setting gain to 8x");
            gpio_put(G0, 0);
            gpio_put(G1, 1);
            gpio_put(G2, 1);
            gpio_put(G3, 0);
            gpio_put(G4, 0);
            break;
        default:
            printf("Error\nDefaulting to a gain of 1x ");
    }
}


// bool 0 for AC coupling and 1 for DC coupling
void tmux_set_coupling(bool state){
    if(state == 0){
        gpio_put(ACDC_SEL, state);
        printf("AC coupled");
    }
    else{
        gpio_put(ACDC_SEL, state);
        printf("Not AC Coupled");
    }
}

void init_frontend(){
    gpio_init(G0);
    gpio_set_dir(G0, 1);

    gpio_init(G1);
    gpio_set_dir(G1, 1);

    gpio_init(G2);
    gpio_set_dir(G2, 1);

    gpio_init(G3);
    gpio_set_dir(G3, 1);

    gpio_init(G4);
    gpio_set_dir(G4, 1);

    pga_set_gain(1); // initial coupling to 
    tmux_set_coupling(1);
}