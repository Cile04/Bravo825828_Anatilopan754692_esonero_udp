/*
 * main.c
 *
 * UDP Client - Template for Computer Networks assignment
 *
 * This file contains the boilerplate code for a UDP client
 * portable across Windows, Linux, and macOS.
 */

#if defined WIN32
#include <winsock.h>
#else

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket close
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "protocol.h"

#define NO_ERROR 0
#define DEFAULT_IP_SV "localhost" //si deve poter cambiare ma altrimenti questo è di default.-
#define TX_BUFFER_SIZE 512
#define RX_BUFFER_SIZE 512

void clearwinsock()
{
#if defined WIN32
    WSACleanup();
#endif
}

void errorhandler(char *error_message)
{
    printf("%s", error_message);
}

// PER METTERE LA PRIMA LETTERA DELLA CITTA' IN MAIUSCOLO SOLO IN STAMPA//
void formatta_citta(char *city)
{
    if (city[0] != '\0')
    {
        city[0] = toupper((unsigned char)city[0]);
    }
}

void stampa_risultato(weather_response_t risposta, char *nome_citta)
{
    formatta_citta(nome_citta);
    switch (risposta.status)
    {
        case VALID_REQ:
        switch(risposta.type){
            case 't': printf("%s: Temperatura = %.1f°C\n", nome_citta, risposta.value); break;
            case 'h': printf("%s: Umidità = %.1f%%\n", nome_citta, risposta.value); break;
            case 'w': printf("%s: Vento = %.1f km/h\n", nome_citta, risposta.value); break;
            case 'p': printf("%s: Pressione = %.1f hPa\n", nome_citta, risposta.value); break;
            default: printf("Dato sconosciuto\n"); break;
        }
        break;

        case INVALID_CITY:
        printf("Città non disponibile\n");
        break;

        case INVALID_REQ:
        printf("Richiesta non valida\n");
        break;
    }
}

int main(int argc, char *argv[])
{
    #if defined WIN32
    SetConsoleOutputCP(65001); 
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != NO_ERROR) {
        printf("Error at WSAStartup()\n");
        return -1;
    }
#endif

    char IP_SV[64] = DEFAULT_IP_SV;
    int port = SERVER_PORT;
    weather_request_t wrichiesta;
    int r_trovato = 0;

    // PER CONTROLLARE CHE I PARAMETRI SIANO ESATTAMENTE 3, ALTRIMENTI, ERRORE //

    if (argc < 3)
    {
        fprintf(stderr, "Il formato per l'utilizzo è: %s [-s server] [-p port] -r \"tipo città\"\n", argv[0]);
        return -1;
    }

    int i = 1;

    // CONTROLLO SU OGNI POSSIBILE PARAMETRO MANCANTE//
    while (i < argc)
    {
        if (strcmp(argv[i], "-s") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "ERRORE: NON hai inserito il valore dopo -s. IP Mancante\n");
                return -1;
            }
            strncpy(IP_SV, argv[i + 1], sizeof(IP_SV) - 1);
            i += 2;
        }
        else if (strcmp(argv[i], "-p") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "ERRORE: NON hai inserito la porta dopo -p\n");
                return -1;
            }
            port = atoi(argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "-r") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "ERRORE: Manca richiesta dopo -r\n");
                return -1;
            }
            char *arg = argv[i + 1];

            if(strchr(arg, '\t')!=NULL){
                fprintf(stderr, "ERRORE: Caratteri di tabulazione NON ammessi.\n");
                return -1;
            }

            char *primo_spazio = strchr(arg, ' ');
            if (primo_spazio == NULL){
                fprintf(stderr, "ERRORE: Formato Invalido, manca la città.\n");
                return -1;
            }

            int type_len = primo_spazio - arg;
            if(type_len != 1){
                fprintf(stderr, "ERRORE: Il tipo deve essere un singolo carattere.\n");
                return -1;
            }

            wrichiesta.type = arg[0];

            char *inzio_citta = primo_spazio;
            while(*inzio_citta != '\0' && isspace((unsigned char)*inzio_citta)) inzio_citta++;

            if(*inzio_citta == '\0'){
                fprintf(stderr, "ERRORE: Nome città mancante.\n");
                return -1;
            }

            if(strlen(inzio_citta) > 63){
                fprintf(stderr, "ERRORE: Nome città troppo lungo.\n");
                return -1;
            }

            strncpy(wrichiesta.city, inzio_citta, sizeof(wrichiesta.city)-1);
            wrichiesta.city[63] = '\0';
            r_trovato = 1;
            i += 2;
        } else {
            fprintf(stderr, "Parametro Sconosciuto: %s\n", argv[i]);
            return -1;
        }
    }

    if (!r_trovato)
    {
        fprintf(stderr, "ERRORE: Il parametro -r è OBBLIGATORIO!\n");
        return -1;
    }

    //RISOLUZUONE DEL DNS//

    struct hostent *host;
    struct in_addr server_addr_struct;

    //RISOLUZIONE DEL NOME//

    host = gethostbyname(IP_SV);
    if(host == NULL){
        unsigned long ip_addr = inet_addr(IP_SV);
        if(ip_addr == INADDR_NONE){
            fprintf(stderr, "ERRORE: Impossibile risolvere il server '%s'\n", IP_SV);
            clearwinsock();
            return -1;
        }

        server_addr_struct.s_addr = ip_addr;
    } else {
        server_addr_struct = *((struct in_addr *)host -> h_addr_list[0]);
    }

    struct hostent *he_reverse;
    char *nome_finale_sv;
    char *ip_finale_sv = inet_ntoa(server_addr_struct);

    he_reverse = gethostbyaddr((const char *)&server_addr_struct, sizeof(server_addr_struct), AF_INET);
    if (he_reverse != NULL) {
        nome_finale_sv = he_reverse->h_name;
    } else {
        nome_finale_sv = IP_SV;
    }

    // Creazione socket

    int my_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (my_socket < 0)
    {
        errorhandler("Creazione socket fallita\n");
        clearwinsock();
        return -1;
    }

    // Dico l'indirizzo a cui connettersi

    struct sockaddr_in sad;
    memset(&sad, 0, sizeof(sad));

    sad.sin_family = AF_INET;
    sad.sin_addr = server_addr_struct;
    sad.sin_port = htons(port);             // Porta del server (56700)

    char buffer_tx[TX_BUFFER_SIZE];
    memset(buffer_tx, 0, TX_BUFFER_SIZE);

    buffer_tx[0] = wrichiesta.type;
    int city_len = strlen(wrichiesta.city);
    memcpy(&buffer_tx[1], wrichiesta.city, city_len);
    int tx_len = 1 + city_len + 1;

    // INVIO PARAMETRI AL SV//

    if (sendto(my_socket, buffer_tx, tx_len, 0, (struct sockaddr *) &sad, sizeof(sad)) != tx_len){
        errorhandler("Sendto ha inviato un numero differente di bytes\n.");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }


    // PER RICEVERE I BYTES E VERIFICHE NEL RICEVERE //

    char buffer_rx[RX_BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    int bytes_rcvd = recvfrom(my_socket, buffer_rx, RX_BUFFER_SIZE, 0, (struct sockaddr*) &from_addr, &from_len);

    if(bytes_rcvd <= 0){
        errorhandler("ERRORE: recvfrom fallita o timeout.\n");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }

    weather_response_t risposta;
    int offset = 0;

    uint32_t net_status;
    memcpy(&net_status, buffer_rx + offset, sizeof(uint32_t));
    risposta.status = ntohl(net_status);
    offset += sizeof(uint32_t);

    memcpy(&risposta.type, buffer_rx + offset, sizeof(char));
    offset += sizeof(char);

    uint32_t net_value_int;
    memcpy(&net_value_int, buffer_rx + offset, sizeof(uint32_t));
    net_value_int = ntohl(net_value_int);
    memcpy(&risposta.value, &net_value_int, sizeof(float));
    offset += sizeof(uint32_t);

    //OUTPUT//

    printf("Ricevuto risultato dal server %s (ip %s). ", nome_finale_sv, ip_finale_sv);

    stampa_risultato(risposta, wrichiesta.city);

    closesocket(my_socket);
    clearwinsock();
    return 0;
    }