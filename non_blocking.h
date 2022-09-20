//
// Created by robert on 15/5/21.
//

#ifndef COMP30023_2021_PROJECT_2_NON_BLOCKING_H
#define COMP30023_2021_PROJECT_2_NON_BLOCKING_H

typedef struct {
    int client_id;
    int dns_id;
} packet_mapper;

typedef struct {
    int dns_socket;
    int client_socket;
} sockets_t;



#endif //COMP30023_2021_PROJECT_2_NON_BLOCKING_H
