#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "font.h"

ssd1306_t ssd; // Estrutura do display

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0);
}

// ==== HANDLER DE INTERRUPÇÃO DO CORE 1 ====
void core1_interrupt_handler()
{
    while (multicore_fifo_rvalid())
    {
        uint32_t val = multicore_fifo_pop_blocking();
        float voltage = val * 3.3 / 4095.0;

        printf("Core 1 (IRQ): Valor RECEBIDO do Core 0: %lu, Voltagem: %.2f V\n", val, voltage);

        // --- Atualiza display ---
        char str_adc[20], str_volt[20];
        sprintf(str_adc, "ADC: %lu", val);
        sprintf(str_volt, "V: %.2f V", voltage);

        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, str_adc, 25, 10);
        ssd1306_draw_string(&ssd, str_volt, 25, 30);
        ssd1306_send_data(&ssd);
    }

    multicore_fifo_clear_irq(); // Limpa a interrupção
}

// ==== FUNÇÃO PRINCIPAL DO CORE 1 ====
void core1_entry()
{
    // Inicializa o display (somente uma vez)
    init_Display(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "DISPLAY OK!", 10, 20);
    ssd1306_send_data(&ssd);
    sleep_ms(1500);

    multicore_fifo_clear_irq();
    irq_set_exclusive_handler(SIO_IRQ_PROC1, core1_interrupt_handler);
    irq_set_enabled(SIO_IRQ_PROC1, true);

    while (true)
    {
        tight_loop_contents();
    }
}

int main()
{
    // Para ser utilizado o modo BOOTSEL com botão B
    stdio_init_all();

    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    printf("Pressione o botao B para entrar em BOOTSEL\n");
    // Fim do trecho para modo BOOTSEL com botão B

    adc_init();
    adc_gpio_init(26);   // Configura GPIO26 como entrada ADC
    adc_select_input(0); // Canal ADC 0 (GPIO26)
    printf("ADC INICIADO\n");

    multicore_launch_core1(core1_entry);
    printf("CORE 1 INICIADO\n");

    while (true)
    {
        uint16_t adc_value = adc_read(); // Lê ADC (12 bits)
        printf("Core 0: Valor lido no (ADC): %u\n", adc_value);
        multicore_fifo_push_blocking(adc_value); // Envia para Core 1
        sleep_ms(250);                           // Aguarda 250 ms
    }
}