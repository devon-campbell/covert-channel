#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#include "utils.h"



int main(){
    Data *data = recv_data(100000000);
    // printf("Received data length: %d\n", data->length);
    for(int i = 0; i < data->length; i++){
        printf("%c", data->data[i]);
    }
    printf("\n");
    free(data->data);
    free(data);
    return 0;
}