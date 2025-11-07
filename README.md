# DoisNucleos - CoreSense  
**Inteligência dividida, resultados unificados.**

## Descrição  
O **CoreSense** é um sistema embarcado desenvolvido na **Raspberry Pi Pico (RP2040)** para monitorar em tempo real **temperatura, umidade e pressão atmosférica**.  
Utilizando dois núcleos de processamento, o projeto divide as tarefas entre coleta e exibição de dados, garantindo **alto desempenho, resposta rápida e eficiência energética**.  

O **Core 0** coleta informações dos sensores **AHT20** e **BMP280** via barramento **I2C**, enquanto o **Core 1** atualiza o **display OLED SSD1306** e controla LEDs indicadores que mostram o estado do sistema.  
Um botão físico permite entrar no **modo BOOTSEL** para atualização de firmware de forma prática e segura.  

Aplicações ideais incluem **monitoramento ambiental**, **automação inteligente** e **sistemas educacionais de IoT**.

---

## Funcionalidades  
- Leitura em tempo real de **temperatura, umidade e pressão**  
- **Divisão de tarefas entre núcleos (multicore)** para melhor desempenho  
- Exibição dos dados no **display OLED SSD1306**  
- **LED verde** indica leituras válidas; **LED vermelho** sinaliza falhas  
- **Modo BOOTSEL** acionável por botão físico para atualização de firmware  
- Comunicação entre núcleos via **FIFO multicore**  

---

## Componentes Utilizados  

### Microcontrolador  
- **Raspberry Pi Pico (RP2040)**  
  - Núcleos: 2x ARM Cortex-M0+  
  - Comunicação multicore via FIFO  

### Sensores  
- **AHT20** — mede temperatura e umidade relativa do ar  
- **BMP280** — mede pressão atmosférica e temperatura  

### Display  
- **OLED SSD1306** (0.96", interface I2C)  
  - Exibe as leituras em tempo real  
  - Controlado pela biblioteca `ssd1306.h`  

### LEDs Indicadores  
- **LED Verde (GPIO 11):** sensores funcionando corretamente  
- **LED Vermelho (GPIO 13):** falha na leitura  

### Botão BOOTSEL  
- **GPIO 6:** permite reinicialização no modo de atualização de firmware  

### Comunicação I2C  
- **Sensores:** SDA = GPIO 0, SCL = GPIO 1  
- **Display:** SDA = GPIO 14, SCL = GPIO 15  
- **Velocidade:** 400 kHz (modo Fast)

---

## Arquitetura Multicore  

| Núcleo | Função Principal | Detalhes |
|--------|------------------|-----------|
| **Core 0** | Leitura dos sensores | Coleta, processa e envia os dados via FIFO |
| **Core 1** | Exibição e controle | Atualiza o display e controla os LEDs |

**Funções principais:**  
- `multicore_fifo_push_blocking()` — envia dados do Core 0 → Core 1  
- `multicore_fifo_pop_blocking()` — recebe dados no Core 1
