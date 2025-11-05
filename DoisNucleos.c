#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "font.h"
#include "bmp280.h"
#include "aht20.h"

ssd1306_t ssd; // Estrutura do display
volatile bool read_sensors_flag = false;

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
#define LED_VERDE 11
#define LED_VERMELHO 13

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
        // Recebe o ponteiro enviado pelo Core 0
        uint32_t addr = multicore_fifo_pop_blocking();
        Data_sensors *ptr = (Data_sensors *)addr;

        printf("Core 1 (IRQ): Dados recebidos via ponteiro:\n");
        printf("  Temperatura: %.2f °C\n", ptr->temperature);
        printf("  Umidade: %.2f %%\n", ptr->humidity);
        printf("  Pressão: %.2f kPa\n", ptr->pressure);

        // --- Atualiza display ---
        char str_temp[32], str_hum[32], str_press[32];
        sprintf(str_temp, "T: %.1fC", ptr->temperature);
        sprintf(str_hum, "H: %.1f%%", ptr->humidity);
        sprintf(str_press, "P: %.1fkPa", ptr->pressure);

        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, str_temp, 25, 10);
        ssd1306_draw_string(&ssd, str_hum, 25, 30);
        ssd1306_draw_string(&ssd, str_press, 25, 50);
        ssd1306_send_data(&ssd);
    }

    multicore_fifo_clear_irq(); // Limpa a interrupção
}

// ==== FUNÇÃO PRINCIPAL DO CORE 1 ====
void core1_entry()
{
    // --- Inicialização dos LEDs ---
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    printf("LEDs inicializados\n");

    gpio_put(LED_VERDE, 0);
    gpio_put(LED_VERMELHO, 0);

    // --- Inicialização do Display ---
    init_Display(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "DISPLAY OK!", 10, 20);
    ssd1306_send_data(&ssd);
    sleep_ms(1000);

    multicore_fifo_clear_irq();
    irq_set_exclusive_handler(SIO_IRQ_PROC1, core1_interrupt_handler);
    irq_set_enabled(SIO_IRQ_PROC1, true);

    while (true)
    {

        if (read_sensors_flag)
        {
            gpio_put(LED_VERDE, 1);
            sleep_ms(500);
            gpio_put(LED_VERDE, 0);
            sleep_ms(500);
        }
        else
        {
            gpio_put(LED_VERMELHO, 1);
            sleep_ms(500);
            gpio_put(LED_VERMELHO, 0);
            sleep_ms(500);
        }
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

    multicore_launch_core1(core1_entry);
    printf("CORE 1 INICIADO\n");

    Data_sensors data_sensors;
    int32_t raw_temp_bmp, raw_pressure_pa_int;
    float g_temp_bmp;

    while (true)
    {
        // --- Leitura dos Sensores ---
        bmp280_read_raw(I2C_PORT_SENSORES, &raw_temp_bmp, &raw_pressure_pa_int);
        g_temp_bmp = bmp280_convert_temp(raw_temp_bmp, &params) / 100.0;
        data_sensors.pressure = bmp280_convert_pressure(raw_pressure_pa_int, raw_temp_bmp, &params) / 1000.0;

        if (data_sensors.pressure <= 0)
            read_sensors_flag = false;

        AHT20_Data data_aht;
        if (aht20_read(I2C_PORT_SENSORES, &data_aht))
        {
            data_sensors.humidity = data_aht.humidity;
            data_sensors.temperature = (g_temp_bmp + data_aht.temperature) / 2.0f;
            read_sensors_flag = true;
        }
        else
        {
            read_sensors_flag = false;
        }

        // --- Envia o endereço da struct para o Core 1 ---
        multicore_fifo_push_blocking((uint32_t)&data_sensors);

        // --- DEBUG SERIAL ---
        printf("Core 0 -> Temp: %.2f C, Umid: %.2f %%, Press: %.2f kPa\n",
               data_sensors.temperature,
               data_sensors.humidity,
               data_sensors.pressure);

        sleep_ms(1000); // Aguarda 1000 ms
    }
}