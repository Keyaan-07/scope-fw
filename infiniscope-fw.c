#include <stdio.h>
#include "pico/stdlib.h"
// not using dma as of now
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "read_adc.pio.h"
#include "gfx.h"

// library link: https://github.com/tvlad1234/pico-displayDrivs
#include "st7789.h"

volatile bool trigger_or_not = 0;
#define capture_length 2048
#define trigpin 1 // define the actual trigger pin here(coming from the comparator)
uint16_t captured_stuff[capture_length];

void int_read_adc(uint pin, uint32_t GPIO_IRQ_EDGE_RISE){
    trigger_or_not = 1; //yes, it triggers.
}



int main()
{
    stdio_init_all();
    init_frontend();

    // next four lines directly taken from pico sdk examples with modifications
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &read_adc_program);
    printf("Loaded program at %d\n", offset);
    uint sm = pio_claim_unused_sm(pio, true);

    read_adc_program_init(pio, sm, offset, 2);
    gpio_set_irq_enabled_with_callback(trigpin, GPIO_IRQ_EDGE_RISE, true, &int_read_adc); // interrupt example taken from the RP2350 datasheet; page 549


    LCD_initDisplay(240, 320);
    LCD_setRotation(0);
    GFX_createFramebuf();



    while (1){
        while(trigger_or_not){
            for(int i = 0; i < capture_length; i++){
                captured_stuff[i] = pio_sm_get_blocking(pio, sm);
            }

            printf("Packet Start\n");
            for(int i = 0; i < capture_length; i++){
                printf("%d\n", captured_stuff[i]);
            }
            printf("Packet Over\n");

            trigger_or_not = 0;
        }

        sleep_ms(10);
    }
    
    return 0;

}