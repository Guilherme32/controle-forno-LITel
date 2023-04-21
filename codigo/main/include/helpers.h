#include "esp_system.h"
#include <string.h>


/**
* Confere se uma string eh um numero. So confere ate o tamanho especificado.
* @param str A string a ser conferida
* @param len O comprimento a ser checado
* @return true se for numero inteiro, false se nao
*/
bool is_number(char* str, int len);
