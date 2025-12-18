/*
 * protocol.h
 *
 * Shared header file for UDP client and server
 * Contains protocol definitions, data structures, constants and function prototypes
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_
#if defined WIN32
    #include <winsock.h>
#else
    #define closesocket close
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netinet/in.h>
#endif

#define SERVER_PORT 56700 
#define BUFFER_SIZE 512
#define QUEUE_SIZE 5

// Codici di stato (Status Codes)
#define VALID_REQ 0     // Successo
#define INVALID_CITY 1  // Città non disponibile
#define INVALID_REQ 2   // Richiesta non valida

typedef struct {
    char type;        // Weather data type: 't', 'h', 'w', 'p'
    char city[64];    // City name (null-terminated string)
} weather_request_t;

typedef struct {
    unsigned int status;  // Response status code
    char type;            // Echo of request type
    float value;          // Weather data value
} weather_response_t;

// Prototipi funzioni meteo
float get_temperature(void);
float get_humidity(void);
float get_wind(void);
float get_pressure(void);

#endif /* PROTOCOL_H_ */