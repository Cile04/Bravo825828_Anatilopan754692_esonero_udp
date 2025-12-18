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
#include "protocol.h"

#define NO_ERROR 0
#define DEFAULT_IP_SV "127.0.0.1"

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

int trovacitta(const char *target)
{
    char *elenco[] = {"bari", "roma", "milano", "napoli", "torino", "palermo", "genova", "bologna", "firenze", "venezia", "sicilia", "bergamo", "latina"};
    int size = 13;

    for (int i = 0; i < size; i++)
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

void gestione_connessione_client(int client_socket, struct sockaddr_in client_addr)
{
    int bytes_ricevuti;
    weather_response_t risposta;
    weather_request_t richiesta;

    // controllo che siano stati ricevuti i bytes
    bytes_ricevuti = recv(client_socket, (char *)&richiesta, sizeof(richiesta), 0);

    if (bytes_ricevuti <= 0)
    {
        printf("Errore in ricezione o connessione chiusa dal client.\n");
        closesocket(client_socket);
        return;
    }

    // Evito Overflow Buffer
    richiesta.city[63] = '\0';

    // per renderlo case insensitive
    for (int i = 0; richiesta.city[i]; i++)
    {
        richiesta.city[i] = tolower((unsigned char)richiesta.city[i]);
    }
    richiesta.type = tolower((unsigned char)richiesta.type);

    printf("Richiesta '%c %s' dal client ip %s\n",
           richiesta.type, richiesta.city, inet_ntoa(client_addr.sin_addr));

    risposta.type = richiesta.type;
    risposta.value = 0.0;

    int controllo_citta = trovacitta(richiesta.city);

    if (controllo_citta == VALID_REQ)
    {
        // Città valida, ora andiamo a validare il tipo
        risposta.status = VALID_REQ;

        switch (richiesta.type)
        {
        case 't':
            risposta.value = get_temperature();
            break;
        case 'h':
            risposta.value = get_humidity();
            break;
        case 'w':
            risposta.value = get_wind();
            break;
        case 'p':
            risposta.value = get_pressure();
            break;
        default:
            // tipo errato
            risposta.status = INVALID_REQ;
            risposta.type = '\0';
            risposta.value = 0.0;
            break;
        }
    }
    else
    {
        // Città non valida
        risposta.status = INVALID_CITY;
        risposta.type = '\0';
        risposta.value = 0.0;
    }

    // risposta in base allo status
    if (risposta.status == VALID_REQ)
    {
        printf("Esito: Successo (0). Il valore generato è: %.2f\n", risposta.value);
    }
    else if (risposta.status == INVALID_REQ)
    {
        printf("Esito: La richiesta NON è valida (Cod errore: 2)\n");
    }
    else if (risposta.status == INVALID_CITY)
    {
        printf("Esito: La città inserita NON è valida (Cod errore: 1)\n");
    }

    risposta.status = htonl(risposta.status);

    if (send(client_socket, (char *)&risposta, sizeof(risposta), 0) != sizeof(risposta))
    {
        errorhandler("ERRORE: send ha inviato un numero di byte diverso a da quello atteso!\n");
        ;
    }else
    {
        printf("Risposta inviata al client correttamente!\n");
    }

    closesocket(client_socket);
    printf("Connessione chiusa\n");
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

    // Creazione Socket

    int my_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (my_socket < 0)
    {
        errorhandler("Creazione socket fallita.\n");
        clearwinsock();
        return -1;
    }

    // Connessione SERVER

    struct sockaddr_in sad;
    memset(&sad, 0, sizeof(sad));

    sad.sin_family = AF_INET;
    sad.sin_addr.s_addr = htonl(INADDR_ANY);
    sad.sin_port = htons(port);

    // binding al socket

    if (bind(my_socket, (struct sockaddr *)&sad, sizeof(sad)) < 0)
    {
        errorhandler("Bind fallito.\n");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }

    // Metto in ascolto il server

    if (listen(my_socket, QUEUE_SIZE) < 0)
    {
        errorhandler("Listen fallito.\n");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }

    srand((unsigned int)time(NULL));
    printf("Server avviato sulla porta %d\n", port);

    // accetto connessione
    struct sockaddr_in cad; // Client Address
    int client_socket;
    socklen_t client_len;

    while (1)
    {
        client_len = sizeof(cad);
        client_socket = accept(my_socket, (struct sockaddr *)&cad, &client_len);

        if (client_socket < 0)
        {
            errorhandler("Accept fallito.\n");
            continue;
        }

        printf("Client Trovato Correttamente!\n");
        printf("L'indirizzo IP del client è: %s\n", inet_ntoa(cad.sin_addr));
        gestione_connessione_client(client_socket, cad);
    }

    closesocket(my_socket);
    clearwinsock();
    return 0;
}
