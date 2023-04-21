#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "driver/gpio.h"


#ifndef WIFI_IMPORT

#define NET_CRED_MAX_LEN 64
typedef struct NetInfo {
  char ssid[NET_CRED_MAX_LEN];
  char password[NET_CRED_MAX_LEN];
  char ip[20];
} NetInfo;

#define WIFI_IMPORT

#endif


/**
* Atualiza as configuracoes do modo station do wifi. Essas sao as credenciais
* com as quais o esp tentara se conectar a rede externa
* @param ssid O SSID da rede externa
* @param password A senha da rede externa
*/
bool update_config(char* ssid, char* password);

/**
* Busca as informacoes do modo station
* @return O struct preenchido com as informacoes do modo station
*/
NetInfo sta_info();

/**
* Busca as informacoes do modo acess point
* @return O struct preenchido com as informacoes do modo acess point
*/
NetInfo ap_info();

/**
* O comando que printa (envia a serial) as informacoes de rede. Printa o IP 
* (caso conectado para o station, sempre para o access point), o ssid e a senha
* para ambas as interfaces (station e access point).
* @param command A string com o texto a ser avaliado como comando. Sera aceito
*                para "netinfo"
* @return true se o comando for aceito, false caso contrario
*/
bool netinfo_command(char* message);

/**
* Printa (envia por serial) a ajuda para o comando netinfo_command.
* @param prefix O prefixo utilizado para os comandos
*/
void netinfo_command_help(char prefix);

/**
* A task responsavel por controlar o led. Led ligado significa station
* conectado. Led piscando significa tentando conectar. Desligado significa
* que nao conseguiu se conectar
*/
void wifi_led_task();

/**
* Inicializa o WiFi
*/
void wifi_init();
