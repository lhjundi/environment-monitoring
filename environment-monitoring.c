#include <stdio.h>
#include "pico/stdlib.h"
#include "dht22.h"
#include "hardware/pwm.h"

#define DHT22_PIN 2
#define SERVO_PIN 3

#define SERVO_PERIOD_MS 20
#define DUTY_INITIAL 1000
#define DUTY_TRIGGER 2000

int temperature_result;
float temperature, humidity;

void setup();
void init_DHT22();
void temperature_monitoring();
bool is_high_temperature();
void toggle_servo(uint32_t gpio, float angle);
void init_pwm_servo(uint gpio);

void init_pwm_servo(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice, 20000);
    pwm_set_clkdiv(slice, 125.0f);
    pwm_set_enabled(slice, true);
}


void toggle_servo(uint32_t gpio, float angle) {
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;
    uint16_t pulso = 600 + (uint16_t)(angle * (1800.0f / 180.0f));
    pwm_set_gpio_level(gpio, pulso);
}


bool is_high_temperature()
{
    return temperature > 30.0;
}

void setup(){
    stdio_init_all();
    init_DHT22();
    init_pwm_servo(SERVO_PIN);
}

void init_DHT22()
{
    temperature_result = dht22_init(DHT22_PIN);
    if (temperature_result != DHT22_OK)
    {
        printf("Erro ao inicializar o sensor DHT22.\n");
        return;
    }
    printf("Leitura do sensor DHT22\n");
}

void temperature_monitoring(bool *servo_triggered)
{
    temperature_result = dht22_read(&temperature, &humidity);   

    if (temperature_result == DHT22_OK)
    {
        printf("Temperatura: %.1f °C | Umidade: %.1f %%\n", temperature, humidity);
        if (is_high_temperature() && !(*servo_triggered))
        {
            *servo_triggered = true;
            toggle_servo(SERVO_PIN, 180.0f);
        }
        else if (!is_high_temperature() && *servo_triggered)
        {
            *servo_triggered = false;
            toggle_servo(SERVO_PIN, 0.0f);
        }
    }
    else
    {
        printf("Erro na leitura do DHT22: código %d\n", temperature_result);
    }
}

int main()
{
    bool servo_triggered = false;

    setup();

    while (1)
    {
        temperature_monitoring(&servo_triggered);
    }
    return 0;
}