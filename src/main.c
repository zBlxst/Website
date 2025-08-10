#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <errno.h>
#include <string.h>
#include <asm-generic/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

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
    address.sin_port = htons(8000);

    if (bind(fdsocket, (struct sockaddr *) &address, sizeof(address)) != 0) {
	    printf("Error during binding: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
    }

    if (listen(fdsocket, BACKLOG) != 0) {
	    printf("Error during listening: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
    }   

    printf("Connection open on port %d\n", PORT);


    for (char again = 'y'; again != 'N'; printf("Again? (y/N)"), again=getc(stdin), getc(stdin), fflush(stdin)) {
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
                write(clientSocket, "HTTP/1.1 500 Not Supported", 26);
                continue;
            }
            
            debug_print_request(req);
            free_request(req);

            
            response* resp = alloc_response();
            char connectionclose[] = "Connection: close";
            char server[] = "Server: homemade";
            char creator[] = "Creator: zblxst";
            char* headers[3] = {connectionclose, server, creator};

            char message[] = "OK";            
            char content[] = "<!DOCTYPE html>\n<html>\n<body>\n\n<h1>This is heading 1</h1>\n<h2>This is heading 2</h2>\n<h3>This is heading 3</h3>\n<h4>This is heading 4</h4>\n<h5>This is heading 5</h5>\n<h6>This is heading 6</h6>\n\n</body>\n</html>\n\n";            


            
            if (build_response(resp, 200, message, headers, 3, content) == -1) {
                continue;
            } 

            debug_print_response(resp);
            send_response(clientSocket, resp);
            free_response(resp);
            
            
            close(clientSocket);
            
        }
    }

    



    close(fdsocket);
    printf("End!\n");
    return 0;
}