#include "serial_comm.h"


// Definitions ------------------------------------------------------------------------------------

#define SERIAL_TAG "SERIAL"
#define MAX_COMMANDS 16
#define PREFIX '!'
#define COMMAND_MAX_LEN 10


// Global variables definition --------------------------------------------------------------------

static bool (*commands[MAX_COMMANDS])(char* message);
static void (*helps[MAX_COMMANDS])(char prefix);
static int commands_len;


// Static functions declaration -------------------------------------------------------------------

/** Checa a mensagem recebida ate o momento com todos os comandos disponiveis. */
static bool check_message(char* message);

/** Coloca um caracter no final de uma string. */
static void append(char new_char, char* str);

/**
* O comando para enviar a ajuda de todos os comandos disponiveis pela serial. A
* assinatura da funcao segue o padrao exigido para todos os comandos.
* @param command A string com o texto a ser avaliado como comando. Sera aceito
*                para "help"
* @return true se o comando for aceito, false caso contrario
*/
static bool help(char* message);

/**
* Printa (envia por serial) a ajuda para o comando help. A assinatura da funcao
* segue o padrao exigido para todas as ajudas de comando.
* @param prefix O prefixo utilizado para os comandos
*/
static void help_help(char prefix);


// Static functions definition --------------------------------------------------------------------

static bool check_message(char* message)
{
    for (int i=0; i<commands_len; i++) {
        if (commands[i](message)) {
            return true;
        }
    }

    return false;
}

static void append(char new_char, char* str)
{
    int len = strlen(str);
    str[len] = new_char;
    str[len + 1] = '\0';
}

static bool help(char* message)
{
    //help
    if (strlen(message) != 4) {
        return false;
    }

    if (strncmp(message, "help", 4) != 0) {
        return false;
    }

    printf("\nCommands available:\n");

    for (int i=0; i<commands_len; i++) {
        printf("-----------------------------------------------------------------------\n");
        helps[i](PREFIX);
    }

    printf("-----------------------------------------------------------------------\n\n");
    printf("You can also interface with the system by accessing the device's IP\n");
    printf("from a web browser if you are connected to a same local network, such\n");
    printf("as the ESP's Soft-AP\n\n");
    printf("For more info, please refer to the main gitHub repo:\n");
    printf("   > https://github.com/Guilherme32/controle-forno-LITel\n");

    return true;
}

static void help_help(char prefix)
{
    printf("%chelp          Displays this message\n", prefix);
}


// Public functions definition --------------------------------------------------------------------

void add_command(bool (*new_command)(char*), void(*command_help)(char))
{
    if (commands_len >= MAX_COMMANDS) {
        ESP_LOGE(SERIAL_TAG, "Error adding command. Max number of commands reached (%d)",
            MAX_COMMANDS);
        return;
    }

    commands[commands_len] = new_command;
    helps[commands_len] = command_help;
    commands_len +=1;
}

void print_header()
{
    printf("--------------------------------------------------------------\n");
    printf("| LITel oven control system v2.0                             |\n");
    printf("| Source code and full manual on the GitHub repo:            |\n");
    printf("| https://github.com/Guilherme32/controle-forno-LITel        |\n");
    printf("| For a quick check on the commands available, type !help    |\n");
    printf("--------------------------------------------------------------\n");
}

void serial_comm_task()
{
    char in_char = '0';
    bool prefixed = false;
    char message[COMMAND_MAX_LEN+1];                        // +1 for the \0

    while (1) {
        int len = uart_read_bytes(UART_NUM_0, (uint8_t*)&in_char, 1, portMAX_DELAY);
        if (len == 0) {
            continue;
        }

        if (in_char == PREFIX) {
            printf("\n");
        }
        uart_write_bytes(UART_NUM_0, &in_char, 1);      // Echo to make it easier to interface
                                            // print doesn't seem to senf without the line break

        if (in_char == PREFIX) {                // Reset when finds the prefix
            message[0] = '\0';
            prefixed = true;
            continue;
        }

        if (strlen(message) >= COMMAND_MAX_LEN) {        // Prints header to help user
            print_header();
            message[0] = '\0';
            prefixed = false;
            continue;
        }

        append(in_char, message);            // Adds the char to the command an checks it
        if (prefixed) {
            if (check_message(message)) {
                message[0] = '\0';
                prefixed = false;
            }
        }
    }
}

void serial_comm_init()
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE 
    };

    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, 256, 256, 0, NULL, 0);

    commands_len = 0;

    add_command(help, help_help);
}

