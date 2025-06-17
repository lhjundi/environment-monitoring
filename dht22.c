/**
 * @file dht22.c
 * @brief Implementação do driver para sensor DHT22/AM2302 no Raspberry Pi Pico
 * 
 * Este arquivo contém a implementação das funções necessárias para comunicação
 * com o sensor de temperatura e umidade DHT22 usando o protocolo de 1-wire.
 */

 #include "dht22.h"
 #include "pico/stdlib.h"
 #include "hardware/gpio.h"
 
 // Constantes de temporização para o protocolo do DHT22
 #define DHT22_START_SIGNAL_DELAY 18000  // Duração do sinal de início (18ms)
 #define DHT22_RESPONSE_WAIT_TIMEOUT 200 // Timeout para aguardar resposta (200μs)
 #define DHT22_BIT_THRESHOLD 50         // Limite para diferenciação entre bit 0 e 1 (50μs)
 #define DHT22_MIN_INTERVAL_MS 2000     // Intervalo mínimo entre leituras (2s)
 
 /**
  * @brief Estrutura para controle do estado do sensor DHT22
  */
 typedef struct {
     uint32_t last_read_time_ms;  // Momento da última leitura realizada
     uint32_t pin;                // Pino GPIO utilizado para comunicação
     bool initialized;            // Flag de inicialização do driver
 } dht22_state_t;
 
 // Estado global do driver
 static dht22_state_t dht22_state = {0, 0, false};
 
 /**
  * @brief Aguarda até que o pino mude para o estado desejado ou ocorra timeout
  * 
  * @param pin Número do pino GPIO
  * @param state Estado esperado (true/false)
  * @param timeout_us Tempo máximo de espera em microssegundos
  * @return 0 se sucesso, -1 se timeout
  */
 static inline int wait_for_pin_state(uint32_t pin, bool state, uint32_t timeout_us) {
     uint32_t start = time_us_32();
     while (gpio_get(pin) != state) {
         if ((time_us_32() - start) > timeout_us) {
             return -1; // Timeout atingido
         }
     }
     return 0; // Estado desejado alcançado
 }
 
 /**
  * @brief Inicializa o driver do DHT22
  * 
  * Configura o pino GPIO e inicializa o estado do driver.
  * 
  * @param pin Número do pino GPIO a ser usado
  * @return DHT22_OK se sucesso
  */
 int dht22_init(uint32_t pin) {
     // Configura o pino GPIO com pull-up interno
     gpio_init(pin);
     gpio_set_pulls(pin, true, false);
     
     // Inicializa a estrutura de estado
     dht22_state.pin = pin;
     dht22_state.last_read_time_ms = 0;
     dht22_state.initialized = true;
     
     return DHT22_OK;
 }
 
 /**
  * @brief Envia o sinal de início da comunicação para o sensor
  * 
  * Gera o sinal de início conforme protocolo do DHT22:
  * - Nível baixo por 18ms
  * - Nível alto por 30μs
  * 
  * @param pin Número do pino GPIO
  * @return DHT22_OK se sucesso
  */
 static int dht22_send_start_signal(uint32_t pin) {
     gpio_set_dir(pin, GPIO_OUT);
     
     // Sequência de início da comunicação
     gpio_put(pin, 0);                         // Nível baixo
     sleep_us(DHT22_START_SIGNAL_DELAY);       // Aguarda 18ms
     gpio_put(pin, 1);                         // Nível alto
     sleep_us(30);                             // Aguarda 30μs
     
     gpio_set_dir(pin, GPIO_IN);               // Muda para entrada
     
     return DHT22_OK;
 }
 
 /**
  * @brief Aguarda e verifica a resposta inicial do sensor
  * 
  * O sensor deve responder com a seguinte sequência:
  * - Nível baixo por 80μs
  * - Nível alto por 80μs
  * - Nível baixo para iniciar transmissão
  * 
  * @param pin Número do pino GPIO
  * @return DHT22_OK se sucesso, DHT22_ERROR_TIMEOUT se falha
  */
 static int dht22_wait_for_response(uint32_t pin) {
     // Verifica a sequência de resposta do sensor
     if (wait_for_pin_state(pin, 0, DHT22_RESPONSE_WAIT_TIMEOUT) != 0) return DHT22_ERROR_TIMEOUT;
     if (wait_for_pin_state(pin, 1, DHT22_RESPONSE_WAIT_TIMEOUT) != 0) return DHT22_ERROR_TIMEOUT;
     if (wait_for_pin_state(pin, 0, DHT22_RESPONSE_WAIT_TIMEOUT) != 0) return DHT22_ERROR_TIMEOUT;
     
     return DHT22_OK;
 }
 
 /**
  * @brief Lê os 40 bits de dados do sensor
  * 
  * O sensor envia 40 bits (5 bytes) no total:
  * - 16 bits para umidade
  * - 16 bits para temperatura
  * - 8 bits para checksum 
  * 
  * Cada bit é codificado pelo tempo que o sinal permanece em nível alto:
  * - ~28μs para bit 0
  * - ~70μs para bit 1
  * 
  * @param pin Número do pino GPIO
  * @param data Buffer para armazenar os dados lidos
  * @return DHT22_OK se sucesso, DHT22_ERROR_TIMEOUT se falha
  */
 static int dht22_read_data(uint32_t pin, uint8_t *data) {
     for (int i = 0; i < 40; i++) {
         // Aguarda início do bit (transição para alto)
         if (wait_for_pin_state(pin, 1, DHT22_RESPONSE_WAIT_TIMEOUT) != 0) return DHT22_ERROR_TIMEOUT;
         
         // Mede duração do pulso em nível alto
         uint32_t pulse_start = time_us_32();
         if (wait_for_pin_state(pin, 0, DHT22_RESPONSE_WAIT_TIMEOUT) != 0) return DHT22_ERROR_TIMEOUT;
         uint32_t pulse_length = time_us_32() - pulse_start;
         
         // Interpreta duração como bit 0 ou 1
         if (pulse_length > DHT22_BIT_THRESHOLD) {
             data[i / 8] |= (1 << (7 - (i % 8))); // Define como bit 1
         }
     }
     
     return DHT22_OK;
 }
 
 /**
  * @brief Verifica o checksum dos dados recebidos
  * 
  * O último byte recebido é um checksum que deve ser igual à soma
  * dos 4 bytes anteriores.
  * 
  * @param data Buffer com os dados recebidos
  * @return DHT22_OK se checksum válido, DHT22_ERROR_CHECKSUM se inválido
  */
 static int dht22_verify_checksum(const uint8_t *data) {
     uint8_t checksum = data[0] + data[1] + data[2] + data[3];
     if (checksum != data[4]) {
         return DHT22_ERROR_CHECKSUM;
     }
     return DHT22_OK;
 }
 
 /**
  * @brief Converte os dados brutos em valores de temperatura e umidade
  * 
  * Formato dos dados:
  * - Bytes 0-1: Umidade * 10 (%)
  * - Bytes 2-3: Temperatura * 10 (°C)
  *   - Bit mais significativo do byte 2 indica sinal negativo
  * 
  * @param data Buffer com os dados brutos
  * @param temperature Ponteiro para armazenar temperatura
  * @param humidity Ponteiro para armazenar umidade
  * @return DHT22_OK se sucesso, DHT22_ERROR_INVALID_DATA se valores inválidos
  */
 static int dht22_convert_data(const uint8_t *data, float *temperature, float *humidity) {
     *humidity = ((data[0] << 8) | data[1]) * 0.1;
     
     *temperature = ((data[2] & 0x7F) << 8 | data[3]) * 0.1;
     if (data[2] & 0x80) {
         *temperature *= -1;
     }
     
     // Verifica se os valores estão dentro dos limites especificados
     if (*humidity < 0.0 || *humidity > 100.0 || *temperature < -40.0 || *temperature > 80.0) {
         return DHT22_ERROR_INVALID_DATA;
     }
     
     return DHT22_OK;
 }
 
 /**
  * @brief Função principal para leitura do sensor DHT22
  * 
  * Realiza a sequência completa de leitura:
  * 1. Verifica inicialização
  * 2. Respeita intervalo mínimo entre leituras
  * 3. Envia sinal de início
  * 4. Aguarda resposta
  * 5. Lê dados
  * 6. Verifica checksum
  * 7. Converte valores
  * 
  * @param temperature Ponteiro para armazenar temperatura
  * @param humidity Ponteiro para armazenar umidade
  * @return DHT22_OK se sucesso ou código de erro apropriado
  */
 int dht22_read(float *temperature, float *humidity) {
     int result;
     uint8_t data[5] = {0};
     
     // Verifica inicialização do driver
     if (!dht22_state.initialized) {
         return DHT22_ERROR_NOT_INITIALIZED;
     }
     
     // Respeita intervalo mínimo entre leituras
     uint32_t current_time = to_ms_since_boot(get_absolute_time());
     if ((current_time - dht22_state.last_read_time_ms) < DHT22_MIN_INTERVAL_MS && 
         dht22_state.last_read_time_ms != 0) {
         sleep_ms(DHT22_MIN_INTERVAL_MS - (current_time - dht22_state.last_read_time_ms));
     }
     
     // Executa sequência de leitura
     result = dht22_send_start_signal(dht22_state.pin);
     if (result != DHT22_OK) return result;
     
     result = dht22_wait_for_response(dht22_state.pin);
     if (result != DHT22_OK) return result;
     
     result = dht22_read_data(dht22_state.pin, data);
     if (result != DHT22_OK) return result;
     
     // Atualiza timestamp da última leitura
     dht22_state.last_read_time_ms = to_ms_since_boot(get_absolute_time());
     
     // Verifica e converte dados
     result = dht22_verify_checksum(data);
     if (result != DHT22_OK) return result;
     
     return dht22_convert_data(data, temperature, humidity);
 }