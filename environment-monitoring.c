#include <stdio.h>
#include "pico/stdlib.h"
#include "dht22.h"

#define DHT22_PIN 2

int temperature_result;
float temperature, humidity;

void setup();
void init_DHT22();
void temperature_monitoring();
bool is_high_temperature();

bool is_high_temperature()
{
    return temperature > 30.0;
}

void setup(){
    stdio_init_all();
    init_DHT22();
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

void temperature_monitoring()
{
    temperature_result = dht22_read(&temperature, &humidity);

    if (temperature_result == DHT22_OK)
    {
        printf("Temperatura: %.1f °C | Umidade: %.1f %%\n", temperature, humidity);
    }
    else
    {
        printf("Erro na leitura do DHT22: código %d\n", temperature_result);
    }
}

int main()
{
    setup();

    while (1)
    {
        temperature_monitoring();
    }
    return 0;
}