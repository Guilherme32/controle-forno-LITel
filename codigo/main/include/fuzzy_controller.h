#include "esp_system.h"
#include "esp_attr.h"


/**
* Roda o passo de controle.
* @param sensor_reading A leitura no sensor de controle
* @param ambient_reading A leitura no sensor de temperatura ambiente
* @param accumulate Se a parcela acumuladore deve ser atualizada nesse passo
* @return O numero de ciclos ligados a ser enviado para a carga (outra forma
          de representar a taxa de potencia)
*/
int IRAM_ATTR run_fuzzy_step(int sensor_reading, int ambient_reading, bool accumulate);

/**
* Atualiza o set point do sistema.
* @param _target O novo set point em termos da leitura esperada
*/
void set_fuzzy_target(int _target);

/**
* Inicializa o sistema de controle.
* @param _last_read Uma primeira leitura do sensor, a ser armazenada na
*                   variavel interna last_read
* @param _max_power O limite superior da potencia permitida, em numero de
*                   ciclos
*/
void fuzzy_init(int _last_read, int _max_power);
