#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

void log_bad_request(char *buffer, int buffer_size) {
    int timestamp = time(NULL);
    int size = strlen(buffer) < buffer_size ? strlen(buffer) : buffer_size;
    char filename[21];
    printf("Time : %d\n", timestamp);
    sprintf(filename, "logs/%x", timestamp);

    int fd = open(filename, O_WRONLY | O_CREAT, 0666);
    write(fd, buffer, buffer_size);
    close(fd);    
}
