#include <stdio.h>
#include "pico/stdlib.h"
// not using dma as of now
#include "hardware/pio.h"

#include "read_adc.pio.h"
int capture_length = 2048;
uint16_t captured_stuff[capture_length];
#define trigpin 2 // define the actual trigger pin here(coming from the comparator)

void int_read_adc(){
    for(int i = 0; i < capture_length; i++){
        capture_length[i] = pio_sm_get_blocking(pio, sm);
    }

    printf("Packet Start\n");
    for(int i = 0; i<capture_length; i++){
        printf("%d\n", capture_length[i]);
    }
    printf("Packet Over\n");
}

int main()
{
    stdio_init_all();

    // next four lines directly taken from pico sdk examples with modifications
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &read_adc_program);
    printf("Loaded program at %d\n", offset);
    uint sm = pio_claim_unused_sm(pio, true);

    read_adc_program_init(pio, sm, offset, 2, 1);
    gpio_set_irq_enabled_with_callback(trigpin, GPIO_IRQ_EDGE_RISE, true, &int_read_adc);
    while (1){
        sleep_ms(100);
    }

    return 0;
}