 #ifndef DHT22_H
 #define DHT22_H
 
 #include <stdint.h>
 
 /**
  * @brief Códigos de retorno das operações do driver
  * 
  * Estas constantes são usadas para indicar o resultado das operações
  * do driver, permitindo tratamento adequado de erros na aplicação.
  */
 #define DHT22_OK 0                        // Operação realizada com sucesso
 #define DHT22_ERROR_CHECKSUM -1           // Falha na verificação do checksum dos dados
 #define DHT22_ERROR_TIMEOUT -2            // Timeout durante a comunicação com o sensor
 #define DHT22_ERROR_INVALID_DATA -3       // Dados recebidos fora dos limites físicos
 #define DHT22_ERROR_NOT_INITIALIZED -4    // Tentativa de uso sem inicialização
 
 /**
  * @brief Inicializa o driver DHT22
  * 
  * Esta função deve ser chamada antes de qualquer tentativa de leitura do sensor.
  * Ela realiza:
  * - Configuração do pino GPIO especificado
  * - Ativação do pull-up interno
  * - Inicialização do estado do driver
  * 
  * @param pin Número do pino GPIO onde o sensor está conectado
  * 
  * @return DHT22_OK se a inicialização for bem-sucedida
  * 
  * @note O pino especificado deve estar conectado ao terminal de dados
  * do sensor DHT22. O sensor também requer alimentação (3.3V-5.5V) e
  * conexão com o terra (GND).
  */
 int dht22_init(uint32_t pin);
 
 /**
  * @brief Realiza uma leitura completa do sensor DHT22
  * 
  * Esta função executa o protocolo completo de comunicação com o sensor:
  * 1. Envia sinal de início
  * 2. Aguarda resposta do sensor
  * 3. Recebe 40 bits de dados (5 bytes)
  * 4. Verifica checksum
  * 5. Converte dados para valores de temperatura e umidade
  * 
  * A função respeita automaticamente o intervalo mínimo de 2 segundos
  * entre leituras, conforme recomendado pelo fabricante. Se chamada
  * antes desse intervalo, aguardará o tempo necessário.
  * 
  * @param temperature Ponteiro para variável onde será armazenada a temperatura
  *                   Valor em graus Celsius, faixa de -40°C a 80°C
  * @param humidity Ponteiro para variável onde será armazenada a umidade
  *                Valor em percentual, faixa de 0% a 100%
  * 
  * @return Um dos seguintes códigos:
  *         - DHT22_OK: Leitura realizada com sucesso
  *         - DHT22_ERROR_CHECKSUM: Dados recebidos corrompidos
  *         - DHT22_ERROR_TIMEOUT: Falha na comunicação com o sensor
  *         - DHT22_ERROR_INVALID_DATA: Valores lidos fora dos limites
  *         - DHT22_ERROR_NOT_INITIALIZED: Driver não inicializado
  * 
  * @note Os valores de temperatura e umidade só são válidos se a função
  * retornar DHT22_OK. Em caso de erro, os valores apontados por
  * temperature e humidity não são modificados.
  * 
  * Exemplo de uso:
  * @code
  * float temp, humid;
  * int result = dht22_read(&temp, &humid);
  * if (result == DHT22_OK) {
  *     printf("Temperatura: %.1f°C, Umidade: %.1f%%\n", temp, humid);
  * } else {
  *     printf("Erro na leitura: %d\n", result);
  * }
  * @endcode
  */
 int dht22_read(float *temperature, float *humidity);
 
 #endif // DHT22_H