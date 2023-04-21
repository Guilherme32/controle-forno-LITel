#include <string.h>
#include "FreeRTOS/freertos.h"
#include "esp_log.h"
#include "driver/uart.h"


/**
* Adiciona um comando serial para a lista.
* @param new_command A funcao do comando. Deve receber o texto a ser processado
*                    como argumento, e retornar true se o comando for aceito,
*                    e false caso contrario.
* @param command_help A funcao que envia pela serial a ajuda correspondente
*                     ao comando. Deve receber como argumento o prefixo
*                     utilizado.
*/
void add_command(bool (*new_command)(char*), void(*command_help)(char));

/**
* Envia pela serial o header do programa. O header contem algumas informacoes
* do programa para auxiliar um usuario no primeiro contato com o sistema.
*/
void print_header();

/**
* A task responsavel por receber as mensagens pela interface serial, e as
* testar com todos os comandos adicionados.
*/
void serial_comm_task();

/**
* Inicializa o sistema de comunicacao serial.
*/
void serial_comm_init();
