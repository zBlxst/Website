#include "struct.h"

#include "utils.h"
#include "string.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

request* alloc_request() {
    request *r = malloc(sizeof(request));
    r->n_header = 0;
    return r;
}

response* alloc_response() {
    response *r = malloc(sizeof(response));
    r->n_header = 0;
    return r;
}

void free_request(request* r) {
    for (int i = 0; i < r->n_header; i++) {
        // printf("Freeing %d/%d -> %p\n", i, r->n_header, r->headers[i]);
        free(r->headers[i]);
    }
    free(r);
}

void free_response(response* r) {
    for (int i = 0; i < r->n_header; i++) {
        // printf("Freeing %d/%d -> %p\n", i, r->n_header, r->headers[i]);
        free(r->headers[i]);
    }
    free(r);
}


int parse_request(request* r, char* request_buffer, int buffer_size) {
    char* new_line;
    char* space;
    
    debug_print("Parsing !\n");
    char *copy = malloc(BUFFER_SIZE);

    // METHOD, PATH and PROTOCOL
    strncpy(copy, request_buffer, BUFFER_SIZE);
    
    space = strchr(copy, ' ');
    if (space == NULL) {
        return -1;
    }
    *space = '\x00';
    strncpy(r->method, copy, METHOD_BUFFER_SIZE);
    copy = space+1;

    space = strchr(copy, ' ');
    if (space == NULL) {
        return -1;
    }
    *space = '\x00';
    strncpy(r->path, copy, PATH_BUFFER_SIZE);
    copy = space+1;
    

    
    new_line = strchr(copy, '\r');
    if (new_line == NULL) {
        return -1;
    }
    *new_line = '\x00';
    *(new_line+1) = '\x00';
    strncpy(r->protocol, copy, PROTOCOL_BUFFER_SIZE);

    copy = new_line+2;

    //Loop over headers
    for (new_line = strchr(copy, '\r'), *new_line = '\x00', *(new_line+1) = '\x00'; new_line != copy; (copy = new_line+2), new_line = strchr(copy, '\r'), *new_line = '\x00', *(new_line+1) = '\x00') {
        if (r->n_header >= MAX_HEADERS) {
            fprintf(stderr, "Too many headers\n");
            return -1;
        }
        int size = strlen(copy)+1 >= HEADER_BUFFER_SIZE ? HEADER_BUFFER_SIZE : strlen(copy)+1;
        char* header_line = malloc(size+1);
        debug_print("Malloc: %p\n", header_line);
        snprintf(header_line, HEADER_BUFFER_SIZE, "%s", copy);
        debug_print("Header : %s\n", header_line);
        header_line[size] = 0;
        r->headers[r->n_header++] = header_line;
    }

    copy = new_line+2;
    strncpy(r->content, copy, CONTENT_BUFFER_SIZE);
    return 0;
}

int get_header_request(request* r, char* name, char* dest) {
    int size = strlen(name);
    if (size + 2 >= HEADER_BUFFER_SIZE) {
        fprintf(stderr, "Too large header");
        return -1;
    }
    char header_name[HEADER_BUFFER_SIZE]; 
    strncpy(header_name, name, HEADER_BUFFER_SIZE-2);
    strcat(header_name, ": ");
    size += 2;

    for (int i = 0; i < r->n_header; i++) {
        if (strncmp(r->headers[i], header_name, size) == 0) {
            strncpy(dest, r->headers[i] + size, HEADER_BUFFER_SIZE-1);
            return 0;
        }
    }
    return -1;
}

void debug_print_request(request* r) {
    if (DEBUG) {
        printf("\nRequest:\n");
        printf("Method : %s\n", r->method);
        printf("Path : %s\n", r->path);
        printf("Protocol : %s\n", r->protocol);
        for (int i = 0; i < r->n_header; i++) {
            printf("%s\n", r->headers[i]);
        }
        printf("\n");
    }
}



int build_response(response* r, int statuscode, char* message, char** headers, int n_header, char* content) {
    if (n_header > MAX_HEADERS - 1) {
        return -1;
    }
    
    sprintf(r->protocol, "HTTP/1.1");
    snprintf(r->statuscode, STATUSCODE_BUFFER_SIZE, "%d", statuscode);
    snprintf(r->message, MESSAGE_BUFFER_SIZE, "%s", message);

    for (int i = 0; i < n_header; i++) {
        int size = strlen(headers[i])+1 >= HEADER_BUFFER_SIZE ? HEADER_BUFFER_SIZE : strlen(headers[i])+1; 
        char* header_line = malloc(size+1);
        debug_print("Malloc: %p\n", header_line);
        snprintf(header_line, HEADER_BUFFER_SIZE, "%s", headers[i]);
        r->headers[i] = header_line;
    }

    r->n_header = n_header;
    snprintf(r->content, CONTENT_BUFFER_SIZE, "%s", content); 

    return 0;
}

void debug_print_response(response* r) {
    if (DEBUG) {
        printf("\nResponse !\n");
        printf("Protocol : %s\n", r->protocol);
        printf("Statuscode : %s\n", r->statuscode);
        printf("Message : %s\n", r->message);
        for (int i = 0; i < r->n_header; i++) {
            printf("%s\n", r->headers[i]);
        }
        printf("Content : %s\n", r->content);
        printf("\n");
    }
}

void send_response(int socket, response* r) {
    char buffer[BUFFER_SIZE];
    char temp[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%s %s %s", r->protocol, r->statuscode, r->message);
    for (int i = 0; i < r->n_header; i++) {
        snprintf(temp, BUFFER_SIZE, "%s\n%s", buffer, r->headers[i]);
        strncpy(buffer, temp, BUFFER_SIZE);
    }
    snprintf(temp, BUFFER_SIZE, "%s\n\n%s", buffer, r->content);
    strncpy(buffer, temp, BUFFER_SIZE);

    int size = strlen(buffer);
    write(socket, buffer, size);
    
}
