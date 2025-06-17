/*

#include <stdio.h>
#include "pico/stdlib.h"
#include "dht22.h"

#define DHT22_PIN 4  // Pino onde o sensor DHT22 está conectado

int main() {
    int result;
    float temperature, humidity;

    stdio_init_all();  // Inicializa comunicação serial
    sleep_ms(2000);    // Aguarda terminal estabilizar

    // Inicializa o sensor DHT22
    result = dht22_init(DHT22_PIN);
    if (result != DHT22_OK) {
        printf("Erro ao inicializar o sensor DHT22.\n");
        return 1;
    }

    printf("Leitura do sensor DHT22\n");

    while (1) {
        result = dht22_read(&temperature, &humidity);

        if (result == DHT22_OK) {
            printf("Temperatura: %.1f °C | Umidade: %.1f %%\n", temperature, humidity);
        } else {
            printf("Erro na leitura do DHT22: código %d\n", result);
        }

        sleep_ms(3000);  // Aguarda 3 segundos antes da próxima leitura
    }

    return 0;
}
*/
/*
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

// --- Configuração do Pino ADC para o Módulo LDR ---
#define ADC_PIN_LDR_AO 26 // Conecte a saída AO do módulo LDR ao GP26 (ADC0)

// --- Parâmetros de Conversão do ADC ---
#define ADC_VREF 3.3f     // Tensão de referência do ADC no Pico (3.3V)
#define ADC_RESOLUTION 4096.0f // Resolução do ADC de 12 bits (2^12 = 4096 valores)

int main() {
    stdio_init_all();

    // --- Inicialização do ADC ---
    adc_init();
    adc_gpio_init(ADC_PIN_LDR_AO);
    adc_select_input(0);

    printf("Iniciando leitura do modulo LDR (apenas AO)\n");

    while (true) {
        uint16_t raw_adc_value = adc_read();
        float voltage_ao = raw_adc_value * (ADC_VREF / ADC_RESOLUTION);

        printf("AO (Valor Bruto): %u, AO (Voltagem): %.2f V\n",
               raw_adc_value, voltage_ao);

        // --- Lógica Corrigida para Inverter a Interpretação ---
        // Se a voltagem está muito baixa, significa que está CLARO (se o seu módulo se comporta assim)
        if (voltage_ao < 0.5f) { // Exemplo de limiar para MUITO CLARO
            printf("  -> Ambiente MUITO CLARO!\n");
        } else if (voltage_ao < 1.5f) { // Exemplo de limiar para CLARO
            printf("  -> Ambiente CLARO.\n");
        } else if (voltage_ao < 2.5f) { // Exemplo de limiar para POUCO ILUMINADO
            printf("  -> Ambiente POUCO ILUMINADO.\n");
        } else { // Se a voltagem está alta, significa que está ESCURO
            printf("  -> Ambiente ESCURO.\n");
        }
        // Fique à vontade para ajustar esses valores (0.5f, 1.5f, 2.5f)
        // com base nas leituras que você observa no seu ambiente.

        sleep_ms(500);
    }

    return 0;
}
*/#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <math.h> // Necessário para log10f e powf

// --- Configuração do Pino ADC ---
#define MQ2_ADC_PIN 27          // Pino conectado à saída analógica do sensor MQ-2 (GP27 = ADC1)
#define ADC_CHANNEL 1           // Canal ADC correspondente ao GP27 (ADC1)

// --- Parâmetros de Conversão do ADC ---
#define ADC_VREF 3.3f           // Tensão de referência do ADC no Pico (Volts)
#define ADC_MAX_VALUE 4095.0f   // Valor máximo do ADC de 12 bits (2^12 - 1)

// --- PARÂMETROS DE CALIBRAÇÃO DO MQ-2 ---
// **ATENÇÃO:** Estes valores são cruciais e devem ser ajustados para SEU SENSOR.

// 1. Resistência de carga (RL) no seu módulo MQ-2.
//    Geralmente 10k Ohm (10000.0f). Verifique no seu módulo.
#define R_LOAD 10000.0f // Ohms

// 2. R0 (Resistência do sensor em AR LIMPO).
//    Este é o valor MAIS IMPORTANTE para a calibração.
//    COMO OBTER R0_CLEAN_AIR:
//    a. Conecte e ligue o sensor MQ-2.
//    b. Deixe-o aquecer por pelo menos 1 minuto em um ambiente com AR REALMENTE LIMPO.
//    c. Anote o valor da 'Tensão' (V_out) que o código imprime, quando estiver estável.
//    d. Calcule R0 usando a fórmula: R0 = R_LOAD * ((V_REF / V_out_limpo) - 1.0f);
//       Ex: Se V_out_limpo for 0.8V e R_LOAD=10000, R0 = 10000 * ((3.3/0.8) - 1) = 31250 Ohms.
//    e. SUBSTITUA o valor abaixo pelo R0 que você calculou.
#define R0_CLEAN_AIR 31250.0f // Exemplo: **AJUSTE ESTE VALOR PARA O SEU SENSOR!**

// 3. Constantes da Curva de Calibração (Rs/R0 vs. PPM) para um GÁS ESPECÍFICO.
//    Estes valores são APENAS EXEMPLOS para GLP/Propano/Metano (gases comuns para MQ-2).
//    Eles vêm da linearização do gráfico do DATASHEET do MQ-2 (curva log-log).
//    Se você quer medir um gás diferente (ex: fumaça de cigarro, Hidrogênio),
//    precisa consultar o datasheet e recalcular 'm_slope' e 'b_intercept' para ESSE GÁS.
//    Para este exemplo, usamos uma linha reta aproximada entre dois pontos do gráfico:
//    Ponto 1: (log10(Rs/R0) para 200 PPM), log10(200 PPM))
//    Ponto 2: (log10(Rs/R0) para 1000 PPM), log10(1000 PPM))
//    Para GLP, valores típicos de Rs/R0 são: ~0.5 para 200 PPM e ~0.25 para 1000 PPM.
#define LOG_RS_R0_200PPM -0.301f // log10(0.5)
#define LOG_200PPM 2.301f        // log10(200)

#define LOG_RS_R0_1000PPM -0.602f // log10(0.25)
#define LOG_1000PPM 3.0f          // log10(1000)

// Calcula a inclinação (m) da linha log-log: m = (y2 - y1) / (x2 - x1)
const float M_SLOPE = (LOG_1000PPM - LOG_200PPM) / (LOG_RS_R0_1000PPM - LOG_RS_R0_200PPM);

// Calcula o intercepto (b) da linha log-log: b = y1 - m * x1
const float B_INTERCEPT = LOG_200PPM - (M_SLOPE * LOG_RS_R0_200PPM);


// --- Funções Auxiliares ---

// Calcula a resistência atual do sensor (Rs)
float calcular_rs(float tensao_ao) {
    // Evita divisão por zero ou valores próximos
    if (tensao_ao < 0.001f) return 10000000.0f; // Retorna um valor muito alto se tensão for zero
    if (tensao_ao >= ADC_VREF) return 0.0f;     // Retorna zero se tensão for VREF (circuito fechado)

    // Fórmula para Rs no divisor de tensão: Rs = RL * ((VCC / Vout) - 1)
    return R_LOAD * ((ADC_VREF / tensao_ao) - 1.0f);
}

// Estima o PPM a partir da razão Rs/R0 usando a curva log-log
float estimar_ppm(float rs_r0_ratio) {
    if (rs_r0_ratio <= 0.0f) return 0.0f; // Evita log de zero ou negativo

    // log10(PPM) = m * log10(Rs/R0) + b
    float log_ppm = (M_SLOPE * log10f(rs_r0_ratio)) + B_INTERCEPT;

    // PPM = 10 ^ (log10(PPM))
    float ppm_value = powf(10.0f, log_ppm);

    // Adiciona limites razoáveis para a faixa do sensor
    if (ppm_value < 0.0f) return 0.0f;
    if (ppm_value > 100000.0f) return 100000.0f; // Limite superior do sensor

    return ppm_value;
}


int main() {
    stdio_init_all();      // Inicializa comunicação serial
    adc_init();            // Inicializa o sistema ADC
    adc_gpio_init(MQ2_ADC_PIN);      // Habilita o pino ADC para o GP27
    adc_select_input(ADC_CHANNEL);   // Seleciona o canal ADC1 (GP27)

    printf("Sensor MQ-2 - Leitura de Gas/Fumaca (PPM Estimado)\n");
    printf("--------------------------------------------------\n");
    printf("1. Ajuste R_LOAD para o valor do resistor no seu modulo MQ-2.\n");
    printf("2. Determine R0_CLEAN_AIR medindo a tensao em ar limpo e calculando Rs.\n");
    printf("3. Para precisao, use constantes M_SLOPE e B_INTERCEPT do datasheet para o gas desejado.\n");
    printf("--------------------------------------------------\n");
    printf("Por favor, aqueça o sensor por ~1 minuto para leituras mais estáveis.\n\n");


    while (1) {
        uint16_t raw_adc_value = adc_read(); // Lê valor bruto (0-4095)
        float tensao = raw_adc_value * ADC_VREF / ADC_MAX_VALUE;

        // Calcula a resistência atual do sensor
        float rs_value = calcular_rs(tensao);
        
        // Calcula a razão Rs/R0
        float rs_r0_ratio = rs_value / R0_CLEAN_AIR;
        
        // Estima o PPM
        float ppm_estimado = estimar_ppm(rs_r0_ratio);

        printf("Leitura bruta: %u | Tensão: %.2f V | Rs: %.0f Ohm | Rs/R0: %.2f | PPM (estimado): %.0f\n",
               raw_adc_value, tensao, rs_value, rs_r0_ratio, ppm_estimado);

        sleep_ms(1000); // Aguarda 1 segundo
    }

    return 0;
}