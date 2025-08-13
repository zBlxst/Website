#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <errno.h>
#include <string.h>
#include <asm-generic/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#include "utils.h"
#include "struct.h"
#include "log.h"


#define PORT 8000

#define BACKLOG 3


int main(int argc, char *argv[]) {

    int fdsocket;
    if ((fdsocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Error during the socket creation : %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    int opt;
    if (setsockopt(fdsocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        fprintf(stderr, "Error during the socket option : %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    srand(time(NULL));
    int port = (rand() % 100) + PORT;
    address.sin_port = htons(port);

    if (bind(fdsocket, (struct sockaddr *) &address, sizeof(address)) != 0) {
	    printf("Error during binding: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
    }

    if (listen(fdsocket, BACKLOG) != 0) {
	    printf("Error during listening: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
    }   

    printf("Connection open on port %d\n", port);


    for (char again = 'y'; again != 'N'; printf("Again? (y/N)"), again=(DEBUG?getc(stdin):'y'), (DEBUG?getc(stdin):0), fflush(stdin)) {
        int clientSocket;
        struct sockaddr_in clientAddress;
        unsigned int addrLen = sizeof(clientAddress);
        if ((clientSocket = accept(fdsocket, (struct sockaddr *) &clientAddress, &addrLen)) != -1) {
            char buffer[BUFFER_SIZE];
            // Convertion de l'IP en texte
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddress.sin_addr), ip, INET_ADDRSTRLEN);
            printf("Connexion de %s:%i\n", ip, clientAddress.sin_port);
            int len = read(clientSocket, buffer, BUFFER_SIZE-1);
            buffer[len] = '\x00';
            printf("%d bytes read\n", len);

            request* req = alloc_request();
            if (parse_request(req, buffer, BUFFER_SIZE)) {
                fprintf(stderr, "Error during parsing\n");
                log_bad_request(buffer, len);
                write(clientSocket, "HTTP/1.1 500 Not Supported", 27);
                continue;
            }
            
            debug_print_request(req);            

            response* resp = alloc_response();
            if (build_file_response(resp, req) == -1) {
                continue;
            }

            debug_print_response(resp);
            send_response(clientSocket, resp);

            free_request(req);
            free_response(resp);

            
            
            close(clientSocket);
            
        }
    }

    
    close(fdsocket);
    printf("End!\n");
    return 0;
}