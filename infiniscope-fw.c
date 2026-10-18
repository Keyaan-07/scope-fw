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

#define LCD_W 240
#define LCD_H 320

#define BTN_PWR 1
#define BTN_UP 1
#define BTN_DN 1
#define BTN_MENU 1
#define BTN_BACK 1
#define BTN_RUN 1

int power_off = 1;

void int_read_adc(uint pin, uint32_t GPIO_IRQ_EDGE_RISE){
    trigger_or_not = 1; //yes, it triggers.
}

void int_PWR_pressed(uint pin, uint32_t GPIO_IRQ_EDGE_FALL){
    if(power_off == 0) power_off = 1; else power_off = 0;
}

void int_UP_pressed(uint pin, uint32_t GPIO_IRQ_EDGE_FALL){

}

void int_DN_pressed(uint pin, uint32_t GPIO_IRQ_EDGE_FALL){

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
    gpio_set_irq_enabled_with_callback(BTN_PWR, GPIO_IRQ_EDGE_FALL, true, &int_PWR_pressed);
    gpio_set_irq_enabled_with_callback(BTN_UP, GPIO_IRQ_EDGE_FALL, true, &int_UP_pressed);
    gpio_set_irq_enabled_with_callback(BTN_DN, GPIO_IRQ_EDGE_FALL, true, &int_DN_pressed);


    LCD_initDisplay(LCD_W, LCD_H);
    LCD_setRotation(1);
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

        for(int i = 0; i<LCD_W; i++){
            int y0 = captured_stuff[i]/capture_length*LCD_H;
            int y1 = captured_stuff[i-1]/capture_length*LCD_H;

            GFX_drawLine(i, y0, i+1, y1, 0xFFFF); // last 16 bits are colour bits
        }
        GFX_flush();

        sleep_ms(10);
    }
    
    return 0;

}