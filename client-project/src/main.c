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
#define DEFAULT_IP_SV "127.0.0.1"

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

void risposta_meteo(char *citta, char tipo, float value)
{
    formatta_citta(citta);
    switch (tipo)
    {
    case 't':
        printf("%s: Temperatura = %.1f°C", citta, value);
        break;
    case 'h':
        printf("%s: Umidità = %.1f%%", citta, value);
        break;
    case 'w':
        printf("%s: Vento = %.1f km/h", citta, value);
        break;
    case 'p':
        printf("%s: Pressione = %.1f hPa", citta, value);
        break;
    default:
        printf("Dato sconosciuto");
        break;
    }
}

int main(int argc, char *argv[])
{
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
            memset(&wrichiesta, 0, sizeof(wrichiesta));

            wrichiesta.type = arg[0]; // per dire che il primo carattere è il tipo

            // ora troviamo il nome, dopo lo spazio//
            int j = 1;
            while (arg[j] != '\0' && isspace((unsigned char)arg[j]))
            {
                j++;
            }

            strncpy(wrichiesta.city, &arg[j], sizeof(wrichiesta.city) - 1);
            wrichiesta.city[63] = '\0'; // evito overflow//

            r_trovato = 1;
            i += 2;
        }
        else
        {
            fprintf(stderr, "Parametro sconosciuto: %s\n", argv[i]);
            return -1;
        }
    }

    if (!r_trovato)
    {
        fprintf(stderr, "ERRORE: Il parametro -r è OBBLIGATORIO!\n");
        return -1;
    }

#if defined WIN32
    SetConsoleOutputCP(65001); // PER GRADI IN TEMPERATURA //
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != NO_ERROR)
    {
        printf("Error at WSAStartup()\n");
        return -1;
    }
#endif

    // Creazione socket

    int my_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

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
    sad.sin_addr.s_addr = inet_addr(IP_SV); // IP del server (localhost)
    sad.sin_port = htons(port);             // Porta del server (56700)

    // Connessione al server

    if (connect(my_socket, (struct sockaddr *)&sad, sizeof(sad)) < 0)
    {
        errorhandler("Errore durante la connessione al server.\n");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }

    // INVIO PARAMETRI AL SV//

    if (send(my_socket, (char *)&wrichiesta, sizeof(wrichiesta), 0) != sizeof(wrichiesta))
    {
        errorhandler("Send ha inviato un numero differente di bytes di quelli attesi");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }

    // PER RICEVERE I BYTES E VERIFICHE NEL RICEVERE //

    int bytes_ricevuti;
    unsigned long int tot_bytes_ricevuti = 0;
    weather_response_t risposta;

    while (tot_bytes_ricevuti < sizeof(risposta))
    {
        bytes_ricevuti = recv(
            my_socket, ((char *)&risposta) + tot_bytes_ricevuti, sizeof(risposta) - tot_bytes_ricevuti, 0);

        if (bytes_ricevuti <= 0)
        {
            errorhandler("ERRORE: recv è fallita o la conessione si è chiusa prematuramente");
            closesocket(my_socket);
            clearwinsock();
            return -1;
        }

        tot_bytes_ricevuti += bytes_ricevuti;
    }
    // SWITCH PER DARE I RISULTATI IN OUTPUT //

    risposta.status = ntohl(risposta.status);

    printf("Ricevuto risultato dal server ip %s. ", IP_SV);

    switch (risposta.status)
    {
    case INVALID_CITY:
        printf("Città non disponibile");
        break;
    case INVALID_REQ:
        printf("Richiesta non valida");
        break;
    case VALID_REQ:
        (risposta_meteo(wrichiesta.city, risposta.type, risposta.value));
        printf("\n");
        break;

    default:
        printf("Erorre del server sconosciuto (STATUS: %d)\n", risposta.status);
    }
    closesocket(my_socket);
    clearwinsock();
    printf("Client terminato");
    return 0;

} // finisce main
