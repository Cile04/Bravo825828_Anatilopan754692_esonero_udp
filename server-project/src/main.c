/*
 * main.c
 *
 * UDP Server - Template for Computer Networks assignment
 *
 * This file contains the boilerplate code for a UDP server
 * portable across Windows, Linux, and macOS.
 */

#if defined WIN32
#include <winsock.h>
typedef int socklen_t;
#else

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h>
#define closesocket close
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "protocol_server.h"

#define NO_ERROR 0
#define TX_BUFFER_SIZE 512

void clearwinsock()
{
#if defined WIN32
    WSACleanup();
#endif
}

float get_temperature(void) {
    // Range: -10.0 to 40.0 °C
    return (-10.0f) + ((float)rand() / (float)RAND_MAX * 50.0f);
}

float get_humidity(void) {
    // Range: 20.0 to 100.0 %
    return 20.0f + ((float)rand() / (float)RAND_MAX * 80.0f);
}

float get_wind(void) {
    // Range: 0.0 to 100.0 km/h
    return ((float)rand() / (float)RAND_MAX * 100.0f);
}

float get_pressure(void) {
    // Range: 950.0 to 1050.0 hPa
    return 950.0f + ((float)rand() / (float)RAND_MAX * 100.0f);
}

int validazione_char_per_citta (const char *city){
    for (int i = 0; city [i] != '\0'; i++){
        if (!isalpha (city[i]) && city[i] != ' '){
            return 0; //quando trova char non valido
        }
    }
    return 1; //successo quindi ok
}

int trovacitta(const char *target)
{
    char *elenco[] = {"bari", "roma", "milano", "napoli", "torino", "palermo", "genova", "bologna", "firenze", "venezia", "sicilia", "bergamo", "latina"};
    int size = 13;

    for (int i = 0; i < size; i++) //per confrinto case insensitive
    {
        if (strcmp(elenco[i], target) == 0)
            return VALID_REQ;
    }
    return INVALID_CITY;
}

void errorhandler(char *errormessage)
{
    printf("%s", errormessage);
}

void richiesta_udp (int socket_server, struct sockaddr_in client_addr, char* buffer_rx, int bytes_ricevuti){

    weather_request_t richiesta;
    weather_response_t risposta;
    char buffer_tx[TX_BUFFER_SIZE]; // UTILIZZO DI BUFFER LOCALE PER DARE LA RISPOSTA

    richiesta.type = buffer_rx[0];
    int city_len = bytes_ricevuti - 1;
    if (city_len >= 64) city_len = 63;
    if(city_len < 0) city_len = 0;
    memcpy(richiesta.city, &buffer_rx[1], city_len);
    richiesta.city[city_len] = '\0';

    struct hostent *he; //he per host end quindi le prime lettere
    char *client_name;
    char *client_ip = inet_ntoa(client_addr.sin_addr);

    he = gethostbyaddr((const char *)&client_addr.sin_addr, sizeof(client_addr.sin_addr), AF_INET);
    client_name = (he != NULL) ? he->h_name : client_ip;

    printf("Richiesta ricevuta da %s (ip %s): type ='%c', city ='%s'\n", client_name, client_ip, richiesta.type, richiesta.city);

    char city_lower[64];
    strcpy(city_lower, richiesta.city);
    for(int i = 0; city_lower[i]; i++) city_lower[i] = tolower((unsigned char) city_lower[i]);
    char type_lower = tolower ((unsigned char) richiesta.type);

    int valid_chars = validazione_char_per_citta(richiesta.city);
    int valid_type = (type_lower=='t'||type_lower=='h'||type_lower=='w'||type_lower=='p');

    risposta.type = richiesta.type;
    risposta.value = 0.0f;

    if(!valid_chars ||!valid_type){
        risposta.status = INVALID_REQ; //ovvero 2
    } else {
        if(trovacitta(city_lower) == INVALID_CITY){
            risposta.status = INVALID_CITY; //OVVERO 1
        } else{
            risposta.status = VALID_REQ; // OVVERO 0
            switch(type_lower){
                case 't': risposta.value = get_temperature(); break;
                case 'h': risposta.value = get_humidity(); break;
                case 'w': risposta.value = get_wind(); break;
                case 'p': risposta.value = get_pressure(); break;
            }
        }
    }

    if (risposta.status == VALID_REQ){
        printf("Esito: Successo (0). Valore : %.2f\n", risposta.value);
    } else if(risposta.status == INVALID_REQ){
        printf("Esito: La richiesta NON è valida (COD Errore: 2)\n");
    } else if(risposta.status == INVALID_CITY){
        printf("Esito: La città che hai inserito NON è valida (COD ERRORE: 1)\n");
    }

    int offset = 0;
    memset(buffer_tx, 0, TX_BUFFER_SIZE);

    uint32_t net_status = htonl(risposta.status);
    memcpy(buffer_tx + offset, &net_status, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(buffer_tx + offset, &risposta.type, sizeof(char));
    offset += sizeof(char);

    uint32_t net_value_int;
    memcpy(&net_value_int, &risposta.value, sizeof(float));
    net_value_int = htonl(net_value_int);
    memcpy(buffer_tx + offset, &net_value_int, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if(sendto(socket_server, buffer_tx, offset, 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) != offset){
        errorhandler("ERRORE: sendto ha inviato un numero diverso di byte da quelli attesi\n");
    } else {
        printf("Risposta inviata al client correttamente!\n");
    }
}

int main(int argc, char *argv[])

{

#if defined WIN32
    // Initialize Winsock
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != NO_ERROR)
    {
        printf("Error at WSAStartup()\n");
        return 0;
    }
#endif

    int port = SERVER_PORT;
    if (argc > 2 && strcmp(argv[1], "-p") == 0)
    {
        port = atoi(argv[2]);
    }

    // Creazione Socket UDP

    int my_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (my_socket < 0)
    {
        errorhandler("Creazione socket fallita.\n");
        clearwinsock();
        return -1;
    }

    // BIND

    struct sockaddr_in sad;
    memset (&sad, 0, sizeof(sad));
    sad.sin_family = AF_INET;
    sad.sin_addr.s_addr = htonl(INADDR_ANY);
    sad.sin_port = htons(port);

    if (bind(my_socket, (struct sockaddr *)&sad, sizeof(sad)) < 0)
    {
        errorhandler("Bind fallito.\n");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }

    srand((unsigned int)time(NULL));
    printf("Server avviato sulla porta %d\n", port);

    struct sockaddr_in cad; //adress del client
    int client_len;
    char buffer_rx[BUFFER_SIZE]; //riceve
    int bytes_ricevuti;

    while (1) //da cambiare domani
    {
        client_len = sizeof(cad);
        memset(buffer_rx, 0, BUFFER_SIZE);

        bytes_ricevuti = recvfrom(my_socket, buffer_rx, BUFFER_SIZE, 0, (struct sockaddr *)&cad, &client_len);

        if(bytes_ricevuti > 0){
            richiesta_udp(my_socket, cad, buffer_rx, bytes_ricevuti);
        }
    }

    closesocket(my_socket);
    clearwinsock();
    return 0;