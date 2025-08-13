#include "struct.h"

#include "utils.h"
#include "string.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

request* alloc_request() {
    request *r = malloc(sizeof(request));
    r->n_header = 0;
    r->content_length = 0;
    return r;
}

response* alloc_response() {
    response *r = malloc(sizeof(response));
    r->n_header = 0;
    r->content_length = 0;
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
    r->content_length = strlen(r->content);
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
        printf("Content : %s\n", r->content);
        printf("Content length : %d\n", r->content_length);
        printf("\n");
    }
}



int build_response(response* r, int statuscode, char* message, char** headers, int n_header, char* content, int content_length) {
    if (n_header + N_BASE_HEADERS > MAX_HEADERS - 1) {
        return -1;
    }
    if (content_length >= CONTENT_BUFFER_SIZE) {
        return -1;
    }

    char* base_headers[N_BASE_HEADERS] = BASE_HEADERS;
    
    sprintf(r->protocol, "HTTP/1.1");
    snprintf(r->statuscode, STATUSCODE_BUFFER_SIZE, "%d", statuscode);
    snprintf(r->message, MESSAGE_BUFFER_SIZE, "%s", message);

    for (int i = 0; i < n_header + N_BASE_HEADERS; i++) {
        char* header;
        if (i < N_BASE_HEADERS) {
            header = base_headers[i];
        } else {
            header = headers[i-N_BASE_HEADERS];
        }
        int size = strlen(header)+1 >= HEADER_BUFFER_SIZE ? HEADER_BUFFER_SIZE : strlen(header)+1; 
        char* header_line = malloc(size+1);
        debug_print("Malloc: %p\n", header_line);
        snprintf(header_line, HEADER_BUFFER_SIZE, "%s", header);
        r->headers[i] = header_line;
    }

    r->n_header = n_header + N_BASE_HEADERS;
    memcpy(r->content, content, content_length);
    r->content_length = content_length;

    return 0;
}

int build_404_response(response* r) {
    return build_response(r, 404, "Not found", NULL, 0, "404 Not found !", 16);
}

int build_file_response(response* resp, request* req) {
    char real_path[PATH_BUFFER_SIZE];
    char* content = malloc(CONTENT_BUFFER_SIZE);
    int bytes_read;
    int fd;
    int path_length;
    int content_size;

    // File content
    path_length = snprintf(real_path, PATH_BUFFER_SIZE, "./web%s", req->path);
    if (strstr(real_path, "..")) {
        return build_response(resp, 403, "Not allowed", NULL, 0, "\"..\" is not allowed, go straight to the file", 45);
    }
    fd = open(real_path, O_RDONLY);
    if (fd < 0) {
        return build_404_response(resp);
    }
    debug_print("Opened file %s\n", real_path);

    char* file_content = malloc(CONTENT_BUFFER_SIZE);
    bytes_read = read(fd, file_content, CONTENT_BUFFER_SIZE);
    close(fd);
    if (bytes_read < 0) {
        return build_response(resp, 500, "Internal Server Error", NULL, 0, "The requested file cannot be read", 34);
    }
    if (bytes_read >= CONTENT_BUFFER_SIZE) {
        return build_response(resp, 500, "Internal Server Error", NULL, 0, "The requested file is too long", 31);
    }
    file_content[bytes_read] = '\x00';
    
    // Header
    fd = open("web/header.html", O_RDONLY);
    if (fd < 0) {
        return build_response(resp, 500, "Internal Server Error", NULL, 0, "Cannot open file header.html", 29);
    }
    char* header_content = malloc(CONTENT_BUFFER_SIZE);
    bytes_read = read(fd, header_content, CONTENT_BUFFER_SIZE);
    close(fd);
    if (bytes_read < 0) {
        return build_response(resp, 500, "Internal Server Error", NULL, 0, "Cannot read header.html", 24);
    }
    if (bytes_read >= CONTENT_BUFFER_SIZE) {
        return build_response(resp, 500, "Internal Server Error", NULL, 0, "header.html is too large", 25);
    }
    header_content[bytes_read] = '\x00';

    // Footer
    fd = open("web/footer.html", O_RDONLY);
    if (fd < 0) {
        return build_response(resp, 500, "Internal Server Error", NULL, 0, "Cannot open file footer.html", 29);
    }
    char* footer_content = malloc(CONTENT_BUFFER_SIZE);
    bytes_read = read(fd, footer_content, CONTENT_BUFFER_SIZE);
    close(fd);
    if (bytes_read < 0) {
        return build_response(resp, 500, "Internal Server Error", NULL, 0, "Cannot read footer.html", 24);
    }
    if (bytes_read >= CONTENT_BUFFER_SIZE) {
        return build_response(resp, 500, "Internal Server Error", NULL, 0, "footer.html is too large", 25);
    }
    footer_content[bytes_read] = '\x00';

    if (strcmp(strstr(real_path+1, "."), ".html") == 0) {
        content_size = snprintf(content, CONTENT_BUFFER_SIZE, "%s\n%s\n%s", header_content, file_content, footer_content);
        if (content_size >= CONTENT_BUFFER_SIZE) {
            return build_response(resp, 500, "Internal Server Error", NULL, 0, "The returned html content is too large", 39);
        }
    } else {
        content_size = snprintf(content, CONTENT_BUFFER_SIZE, "%s", file_content);
    }

    return build_response(resp, 200, "OK", NULL, 0, content, content_size);
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
        printf("Content length : %d\n", r->content_length);
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
    int size_without_content = strlen(buffer);
    snprintf(temp, BUFFER_SIZE, "%s\n\n%s", buffer, r->content);
    snprintf(buffer, BUFFER_SIZE, "%s", temp);
    
    // +2 for the '\n's
    int size = size_without_content + r->content_length + 2;
    
    write(socket, buffer, size);
    
}
