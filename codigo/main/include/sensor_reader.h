#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "esp_timer.h"


/**
* Inicializa o leitor de sensores.
*/
void sensor_init();

/**
* Busca a leitura de um unico sensor.
* @param index O indice do sensor
* @return A leitura do sensor (entre 0 e 1023)
*/
int get_reading(int index);

/**
* Busca a temperatura em um unico sensor.
* @param index O indice no sensor
* @return A temperatura inteira (truncada) no sensor
*/
int get_temperature(int index);

/**
* O comando para fazer o equivalente a funcao get_reading, porem pela serial.
* @param message A string com o texto a ser avaliado como comando. Sera aceito
*                para "sensorX"
* @return true se o comando for aceito, false caso contrario
*/
bool reading_command(char* message);

/**
* Printa (envia por serial) a ajuda para o comando reading_command.
* @param prefix O prefixo utilizado para os comandos
*/
void reading_command_help(char prefix);

/**
* O comando para fazer o equivalente a funcao get_temperature, porem pela
* serial.
* @param message A string com o texto a ser avaliado como comando. Sera aceito
*                para "tempX"
* @return true se o comando for aceito, false caso contrario
*/
bool temperature_command(char* message);

/**
* Printa (envia por serial) a ajuda para o comando temperature_command.
* @param prefix O prefixo utilizado para os comandos
*/
void temperature_command_help(char prefix);

/**
* A task responsavel por fazer a leitura periodica do sensor.
*/
void sensors_task();

