#include <stdio.h>
#include "pico/stdlib.h"
// not using dma as of now
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "read_adc.pio.h"
#include "gfx.h"
#include "frontend_control.h"
#include "hardware/dma.h"

// library link: https://github.com/tvlad1234/pico-displayDrivs
#include "st7789.h"

volatile bool trigger_or_not = 0;
#define capture_length 2048
#define trigpin 1 // define the actual trigger pin here(coming from the comparator)
uint16_t captured_stuff[capture_length];

#define LCD_W 240
#define LCD_H 320
#define TOP_STUFF 40

#define BTN_PWR 1
#define BTN_UP 1
#define BTN_DN 1
#define BTN_MENU 1
#define BTN_BACK 1
#define BTN_RUN 1

int power_off = 1;
int up_pressed = 0;
int dn_pressed = 0;
int menu_state = 0;
int back_pressed = 0;
int run_pressed = 0;
int pga_multiplier = 0;
float clock_divider = 150.0;
bool coupling_state = 1;

void int_read_adc(uint pin, uint32_t GPIO_IRQ_EDGE_RISE){
    trigger_or_not = 1; //yes, it triggers.
}

void int_PWR_pressed(uint pin, uint32_t GPIO_IRQ_EDGE_FALL){
    if(power_off == 0) power_off = 1; else power_off = 0;
}

void int_UP_pressed(uint pin, uint32_t GPIO_IRQ_EDGE_FALL){
    up_pressed = 1;
}

void int_DN_pressed(uint pin, uint32_t GPIO_IRQ_EDGE_FALL){
    dn_pressed = 0;
}

void int_MENU_pressed(uint pin, uint32_t GPIO_IRQ_EDGE_FALL){
    if(menu_state <=1) menu_state++; else menu_state = 0; // gives 3 menu states, 0, 1 and 2
}

void int_BACK_pressed(uint pin, uint32_t GPIO_IRQ_EDGE_FALL){
    back_pressed = 1;
}

void int_RUN_pressed(uint pin, uint32_t GPIO_IRQ_EDGE_FALL){
    run_pressed = 1;
}

void draw_waveform(){
    for(int i = 0; i<LCD_H-TOP_STUFF; i++){
        int y0 = captured_stuff[i]*(LCD_H-TOP_STUFF)/capture_length;
        int y1 = captured_stuff[i-1]*(LCD_H-TOP_STUFF)/capture_length;
        GFX_drawLine(i, y0, i+1, y1, 0xFFFF); // last 16 bits are colour bits
    }
    GFX_setCursor(1, 1);
    GFX_printf("Clock Divison: %.2f\n", clock_divider);
    GFX_setCursor(1, 10);
    GFX_printf("PGA Gain: %.2f\n", pga_multiplier);
    GFX_setCursor(1, 19);
    GFX_printf("coupling state: %s", (coupling_state==0?"AC":"DC"));
    GFX_flush();
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

    // starting basic DMA from pico sdk examples instead of relying on CPU for continously pulling data from PIO. 
    int chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, true);
    
    dma_channel_configure(chan, &c, captured_stuff, &pio->rxf[sm], capture_length, false);
    
    pio_sm_set_clkdiv(pio, sm, clock_divider);

    read_adc_program_init(pio, sm, offset, 2);
    gpio_set_irq_enabled_with_callback(trigpin, GPIO_IRQ_EDGE_RISE, true, &int_read_adc); // interrupt example taken from the RP2350 datasheet; page 549
    gpio_set_irq_enabled_with_callback(BTN_PWR, GPIO_IRQ_EDGE_FALL, true, &int_PWR_pressed);
    gpio_set_irq_enabled_with_callback(BTN_UP, GPIO_IRQ_EDGE_FALL, true, &int_UP_pressed);
    gpio_set_irq_enabled_with_callback(BTN_DN, GPIO_IRQ_EDGE_FALL, true, &int_DN_pressed);
    gpio_set_irq_enabled_with_callback(BTN_BACK, GPIO_IRQ_EDGE_FALL, true, &int_BACK_pressed);
    gpio_set_irq_enabled_with_callback(BTN_RUN, GPIO_IRQ_EDGE_FALL, true, &int_RUN_pressed);
    gpio_set_irq_enabled_with_callback(BTN_MENU, GPIO_IRQ_EDGE_FALL, true, &int_MENU_pressed);


    LCD_initDisplay(LCD_W, LCD_H);
    LCD_setRotation(1);
    GFX_createFramebuf();

    while (1){
        while(trigger_or_not){
            // for(int i = 0; i < capture_length; i++){
            //     captured_stuff[i] = pio_sm_get_blocking(pio, sm);
            // }

            printf("Packet Start\n");
            for(int i = 0; i < capture_length; i++){
                printf("%d\n", captured_stuff[i]);
            }
            printf("Packet Over\n");

            trigger_or_not = 0;
        }
        draw_waveform();
        if(up_pressed == 1 || dn_pressed == 1){
            if(menu_state == 0){
                if(up_pressed == 1) clock_divider = clock_divider*10.0;
                if(dn_pressed == 1) clock_divider = clock_divider/10.0;
                if(clock_divider < 1.0 || clock_divider > 10) clock_divider = 1;
                pio_sm_set_enabled(pio, sm, false);
                pio_sm_set_clkdiv(pio, sm, clock_divider);
                pio_sm_clear_fifos(pio, sm);
                pio_sm_set_enabled(pio, sm, true);

            }
            else if(menu_state == 1){
                if(up_pressed == 1) pga_multiplier = pga_multiplier*2;
                if(dn_pressed == 1) pga_multiplier = pga_multiplier/2;
                pga_set_gain(pga_multiplier);
            }
            else if(menu_state == 2){
                if(up_pressed == 1) coupling_state = 1;
                if(dn_pressed == 1) coupling_state = 0;
                tmux_set_coupling(coupling_state);
            }
            dn_pressed = 0;
            up_pressed = 0;
        }
        sleep_ms(1);
    }

    return 0;

}