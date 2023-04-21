#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "fuzzy_controller.h"


#ifndef CONTROLLER_IMPORT

typedef struct ControllerInfo {
    bool steady;
    int power_ratio[2];
    bool control;
    int target;
} ControllerInfo;

#define CONTROLLER_IMPORT

#endif


/**
* Atualiza a potencia e termina o modo de controle.
* @param cycles_on O numero de ciclos ativos para a nova potencia
*/
void set_ratio(int cycles_on);

/**
* Atualiza o set point e inicia o modo de controle.
* @param _target O novo set point em termos da leitura esperada
*/
void set_target(int _target);

/**
* Desliga o controlador.
*/
void controller_shutdown();

/**
* Busca as informações do controlador.
* @return O struct preenchido com as informacoes
*/
ControllerInfo controller_info();

/**
* O comando para fazer o equivalente a funcao set_ratio, porem pela serial
* @param command A string com o texto a ser avaliado como comando. Sera aceito
*                para "setpXXX"
* @return true se o comando for aceito, false caso contrario
*/
bool set_ratio_command(char* command);

/**
* Printa (envia por serial) a ajuda para o comando set_ratio_command.
* @param prefix O prefixo utilizado para os comandos
*/
void set_ratio_command_help(char prefix);

/**
* O comando para fazer o equivalente a funcao set_target, porem pela serial
* @param command A string com o texto a ser avaliado como comando. Sera aceito
*                para "settXXX.X"
* @return true se o comando for aceito, false caso contrario
*/
bool set_target_command(char* command);

/**
* Printa (envia por serial) a ajuda para o comando set_target_command.
* @param prefix O prefixo utilizado para os comandos
*/
void set_target_command_help(char prefix);

/**
* Inicializa o controlador.
* @param _read_sensor A funcao que retorna a leitura do sensor de controle (S5)
* @param _read_ambient A funcao que retorna a leitura do sensor de temperatura
                       ambiente (S6)
*/
void controller_init(int (*_read_sensor)(), int (*_read_ambient)());
