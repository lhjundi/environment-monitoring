#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "dht22.h"
#include <stdio.h>
#include <string.h>

// GPIOs
#define DHT_PIN 2
#define LDR_PIN 26
#define MQ2_PIN 27
#define SERVO_PIN 3
#define LED_PIN 4
#define RELE_PIN 5


// Converte valor ADC para tensão (0 - 3.3V)
float adc_to_voltage(uint16_t value) {
    return (value * 3.3f) / 4095.0f;
}

// Controla o servo com PWM (0° a 180°)
void set_servo_angle(uint32_t gpio, float angle) {
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;
    uint16_t pulso = 600 + (uint16_t)(angle * (1800.0f / 180.0f));
    pwm_set_gpio_level(gpio, pulso);
}

void init_pwm_servo(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice, 20000); // 20ms period (50Hz)
    pwm_set_clkdiv(slice, 125.0f); // 125MHz / 125 = 1MHz
    pwm_set_enabled(slice, true);
}

int main() {
    stdio_init_all();

    // GPIOs
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    gpio_init(RELE_PIN);
    gpio_set_dir(RELE_PIN, GPIO_OUT);
    gpio_put(RELE_PIN, 0);

    dht22_init(DHT_PIN);
    init_pwm_servo(SERVO_PIN);

    // Inicializa ADC
    adc_init();
    adc_gpio_init(LDR_PIN);
    adc_gpio_init(MQ2_PIN);

    while (1) {
        // --- Temperatura ---        
        float temp = 0.0f, umid = 0.0f;
        char motor[30] = "";
        char ilum[20] = "";
        char alarm[20] = "";
        int resultado = dht22_read(&temp, &umid);
        
        if (temp > 30.0) {
            set_servo_angle(SERVO_PIN, 180.0f); // Alerta físico
            strcpy(motor, "\t --- MOTOR ACIONADO!");
        } else {
            set_servo_angle(SERVO_PIN, 0.0f);
        }

        // --- LDR ---
        adc_select_input(0); // GPIO 26
        uint16_t ldr_raw = adc_read();
        float ldr_v = adc_to_voltage(ldr_raw);
        if (ldr_v < 1.5) { // Limiar de luminosidade baixa
            gpio_put(LED_PIN, 1);
            strcpy(ilum, "\t --- LED ACIONADO!");
        } else {
            gpio_put(LED_PIN, 0);
        }

        // --- MQ-2 ---
        adc_select_input(1); // GPIO 27
        uint16_t mq2_raw = adc_read();
        float mq2_percent = (mq2_raw/4095.0f)*100.0f;
        if (mq2_percent > 50.0) { // Limiar de gás detectado
            gpio_put(RELE_PIN, 1);
            strcpy(alarm, "\t --- ALARME ACIONADO!");
        } else {
            gpio_put(RELE_PIN, 0);
        }
        printf("Temperatura: %.2f °C %s\n", temp, motor);
        printf("Luminosidade: %.2f \n", ldr_v);
        printf("Gás: %.2f %%\n", mq2_percent);
        printf("----------------------------\n");
        sleep_ms(1000);
    }
}
