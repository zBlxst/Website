#ifndef STRUCT_H
#define STRUCT_H

#define METHOD_BUFFER_SIZE 16
#define PATH_BUFFER_SIZE 1024
#define PROTOCOL_BUFFER_SIZE 16

#define STATUSCODE_BUFFER_SIZE 8
#define MESSAGE_BUFFER_SIZE 1024
#define CONTENT_BUFFER_SIZE 1048576


#define HEADER_BUFFER_SIZE 1024
#define MAX_HEADERS 100

#define CONNECTION_CLOSED_HEADER "Connection: close"
#define SERVER_HEADER "Server: homemade"
#define CREATOR_HEADER "Creator: zblxst"

#define BASE_HEADERS {CONNECTION_CLOSED_HEADER, SERVER_HEADER, CREATOR_HEADER}
#define N_BASE_HEADERS 3

typedef struct {

    char method[METHOD_BUFFER_SIZE];
    char path[PATH_BUFFER_SIZE];
    char protocol[PROTOCOL_BUFFER_SIZE];

    char* headers[MAX_HEADERS];
    int n_header;

    char content[CONTENT_BUFFER_SIZE];
    int content_length;

} request;

typedef struct {
    char protocol[PROTOCOL_BUFFER_SIZE];
    char statuscode[STATUSCODE_BUFFER_SIZE];
    char message[MESSAGE_BUFFER_SIZE];

    char* headers[MAX_HEADERS];
    int n_header;

    char content[CONTENT_BUFFER_SIZE];
    int content_length;


} response;


request* alloc_request();
void free_request(request* r);
response* alloc_response();
void free_response(response* r);

int parse_request(request* r, char* request_buffer, int buffer_size);
int get_header_request(request* r, char* name, char* dest);
void debug_print_request(request* r);

int build_response(response* r, int statuscode, char* message, char **headers, int n_header, char *content, int content_length);
int build_404_response(response* r);
int build_file_response(response* resp, request* req);

void debug_print_response(response* r);
void send_response(int socket, response* r);


#endif