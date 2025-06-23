/**
 * @file environment-monitoring.c
 * @brief Environment monitoring system for Raspberry Pi Pico using DHT22, MQ2, LDR, Servo, Relay, and LED.
 *
 * This program monitors temperature, humidity, gas/smoke, and light intensity using various sensors.
 * It controls a servo motor, relay, and an LED based on sensor readings.
 *
 * Features:
 * - Reads temperature and humidity from a DHT22 sensor.
 * - Reads gas/smoke levels from an MQ2 sensor (via ADC).
 * - Reads light intensity from an LDR (via ADC).
 * - Activates a servo motor when high temperature is detected.
 * - Activates a relay when high gas/smoke levels are detected.
 * - Turns on a red LED when light intensity exceeds a threshold.
 *
 * Pin assignments:
 * - DHT22_PIN: GPIO 2 (DHT22 sensor data)
 * - SERVO_PIN: GPIO 3 (Servo PWM control)
 * - MQ2_PIN: GPIO 27 (MQ2 sensor analog output, ADC1)
 * - RELE_PIN: GPIO 5 (Relay control)
 * - LDR_PIN: GPIO 26 (LDR analog output, ADC0)
 * - RED_LED_PIN: GPIO 4 (Red LED)
 *
 * Functions:
 * - setup(): Initializes all peripherals and sensors.
 * - init_DHT22(): Initializes the DHT22 sensor.
 * - setup_adc(): Initializes ADC and configures ADC pins.
 * - setup_led(): Initializes the red LED GPIO.
 * - setup_rele(): Initializes the relay GPIO.
 * - init_pwm_servo(uint gpio): Initializes PWM for servo control.
 * - toggle_servo(uint32_t gpio, float angle): Sets servo to a specific angle.
 * - temperature_monitoring(bool *servo_triggered): Reads temperature/humidity and controls the servo.
 * - ldr_monitoring(): Reads LDR value and controls the red LED.
 * - mq2_monitoring(): Reads MQ2 value and controls the relay.
 * - is_high_temperature(): Checks if the temperature exceeds the threshold.
 * - turn_on_red_led(), turn_off_red_led(): Controls the red LED.
 *
 * Main loop:
 * - Continuously monitors sensors and actuates outputs accordingly.
 *
 * Dependencies:
 * - pico/stdlib.h
 * - dht22.h (external DHT22 driver)
 * - hardware/pwm.h
 * - hardware/adc.h
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "dht22.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

#define DHT22_PIN 2
#define SERVO_PIN 3
#define MQ2_PIN 27
#define MQ2_ADC_CHANNEL 1
#define RELE_PIN 5
#define LDR_PIN 26 // GPIO 26 is ADC0
#define RED_LED_PIN 4

#define LDR_THRESHOLD 1500

int temperature_result;
uint16_t ldr_value, mq2_value;
float temperature, humidity;

void setup();
void init_DHT22();
void setup_adc();
void temperature_monitoring(bool *servo_triggered);
void ldr_monitoring();
void mq2_monitoring(); 
bool is_high_temperature();
void toggle_servo(uint32_t gpio, float angle);
void init_pwm_servo(uint gpio);
void turn_on_red_led();
void turn_off_red_led();

void turn_off_red_led() {
    gpio_put(RED_LED_PIN, 0);
}

void turn_on_red_led() {
    gpio_put(RED_LED_PIN, 1);
}


void ldr_monitoring()
{
    adc_select_input(LDR_PIN - 26); 
    ldr_value = adc_read();
    float ldr_voltage = (ldr_value * 3.3f) / 4095.0f; 
    printf("LDR: %.2f V (Raw: %d)\n", ldr_voltage, ldr_value);
    if (ldr_value > LDR_THRESHOLD)
    {
        turn_on_red_led();
    }
    else
    {
        turn_off_red_led();
    }
    
}

void setup_led(){
    gpio_init(RED_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);
    gpio_put(RED_LED_PIN, 0);
}

void setup_rele(){
    gpio_init(RELE_PIN);
    gpio_set_dir(RELE_PIN, GPIO_OUT);
    gpio_put(RELE_PIN, 0);
}

void setup_adc(){
    adc_init();
    adc_gpio_init(LDR_PIN);
    adc_gpio_init(MQ2_PIN);
}

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
    setup_adc();
    setup_led();
    setup_rele();
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

void mq2_monitoring() {
    adc_select_input(MQ2_ADC_CHANNEL);
    mq2_value = adc_read();
    float mq2_voltage = (mq2_value * 3.3f) / 4095.0f; 
    printf("MQ2: %.2f V (Raw: %d)\n", mq2_voltage, mq2_value);

    if (mq2_value > 2000) {
        gpio_put(RELE_PIN, 1); 
        printf("Alarme ativado!\n");
    } else {
        gpio_put(RELE_PIN, 0);
        printf("Alarme desativado.\n");
    }
}

    
int main()
{
    bool servo_triggered = false;

    setup();

    while (1)
    {
        temperature_monitoring(&servo_triggered);
        ldr_monitoring();
        mq2_monitoring();
    }
    return 0;
}