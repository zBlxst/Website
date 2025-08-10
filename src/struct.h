#ifndef STRUCT_H
#define STRUCT_H

#define METHOD_BUFFER_SIZE 16
#define PATH_BUFFER_SIZE 1024
#define PROTOCOL_BUFFER_SIZE 16

#define STATUSCODE_BUFFER_SIZE 8
#define MESSAGE_BUFFER_SIZE 1024
#define CONTENT_BUFFER_SIZE 1024


#define HEADER_BUFFER_SIZE 1024
#define MAX_HEADERS 100

typedef struct {

    char method[METHOD_BUFFER_SIZE];
    char path[PATH_BUFFER_SIZE];
    char protocol[PROTOCOL_BUFFER_SIZE];

    char* headers[MAX_HEADERS];
    int n_header;

    char content[CONTENT_BUFFER_SIZE];

} request;

typedef struct {
    char protocol[PROTOCOL_BUFFER_SIZE];
    char statuscode[STATUSCODE_BUFFER_SIZE];
    char message[MESSAGE_BUFFER_SIZE];

    char* headers[MAX_HEADERS];
    int n_header;

    char content[CONTENT_BUFFER_SIZE];


} response;


request* alloc_request();
void free_request(request* r);
response* alloc_response();
void free_response(response* r);

int parse_request(request* r, char* request_buffer, int buffer_size);
int get_header_request(request* r, char* name, char* dest);
void debug_print_request(request* r);

int build_response(response* r, int statuscode, char* message, char **headers, int n_header, char *content);
void debug_print_response(response* r);
void send_response(int socket, response* r);


#endif