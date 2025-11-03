#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "font.h"
#include "bmp280.h"
#include "aht20.h"

ssd1306_t ssd; // Estrutura do display

typedef struct
{
    float temperature;
    float humidity;
    float pressure;
} Data_sensors;

// Pinos I2C para os sensores
#define I2C_PORT_SENSORES i2c0
#define I2C_SDA_SENSORES 0
#define I2C_SCL_SENSORES 1

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

    // --- Inicialização dos Sensores (BMP280 e AHT20) ---
    i2c_init(I2C_PORT_SENSORES, 400 * 1000);
    gpio_set_function(I2C_SDA_SENSORES, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_SENSORES, GPIO_FUNC_I2C);
    aht20_init(I2C_PORT_SENSORES);
    printf("AHT20 inicializado\n");

    bmp280_init(I2C_PORT_SENSORES);
    struct bmp280_calib_param params;
    bmp280_get_calib_params(I2C_PORT_SENSORES, &params);
    printf("BMP280 inicializado\n");
    // --- ADC ---
    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);
    printf("ADC inicializado\n");

    multicore_launch_core1(core1_entry);
    printf("CORE 1 INICIADO\n");

    Data_sensors data_sensors;
    int32_t raw_temp_bmp, raw_pressure_pa_int;
    float g_temp_bmp, g_temp_aht, g_temp_media, g_pressao_kpa;

    while (true)
    {

        // --- Leitura dos Sensores ---
        bmp280_read_raw(I2C_PORT_SENSORES, &raw_temp_bmp, &raw_pressure_pa_int);
        g_temp_bmp = bmp280_convert_temp(raw_temp_bmp, &params) / 100.0;
        data_sensors.pressure = bmp280_convert_pressure(raw_pressure_pa_int, raw_temp_bmp, &params) / 1000.0;

        AHT20_Data data_aht;
        if (aht20_read(I2C_PORT_SENSORES, &data_aht))
        {
            g_temp_aht = data_aht.temperature;
            data_sensors.humidity = data_aht.humidity;
        }
        data_sensors.temperature = (g_temp_bmp + g_temp_aht) / 2.0f;

        // DEBUG: Imprime os valores lidos no monitor serial
        printf("BMP280 -> Temp: %.2f C, Pressao: %.2f kPa\n", g_temp_bmp, data_sensors.pressure);
        printf("AHT20  -> Temp: %.2f C, Umidade: %.2f %%\n", g_temp_aht, data_sensors.humidity);
        printf("Media  -> Temp: %.2f C\n\n", data_sensors.temperature);

        // --- Leitura do ADC e envio para o Core 1 ---
        uint16_t adc_value = adc_read();
        printf("Core 0: Valor lido no ADC: %u\n", adc_value);
        multicore_fifo_push_blocking(adc_value);

        sleep_ms(1000); // Aguarda 250 ms
    }
}