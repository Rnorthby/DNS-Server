//
// Created by robert on 10/5/21.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "dns.h"


int read(char * buffer, int read_size){
    dns_message * message;
    memcpy(message, buffer, 12);
    message->header->id = ntohs(message->header->id );
    message->header->qd_count = ntohs(message->header->qd_count);
    message->header->an_count = ntohs(message->header->an_count);
    message->header->ns_count = ntohs(message->header->ns_count);
    message->header->ar_count = ntohs(message->header->ar_count);
    dns_header_type * header = message->header;
    printf ("ID: %d\n", header->id);
    printf ("qr: %d\n", header->qr);
    printf ("opcode: %d\n", header->opcode);
    printf ("aa: %d\n", header->aa);
    printf ("tc: %d\n", header->tc);
    printf ("rd: %d\n", header->rd);
    printf ("ra: %d\n", header->ra);
    printf ("z: %d\n", header->z);
    printf ("rcode: %d\n", header->rcode);

    printf ("qdcount: %d\n", header->qdcount);
    printf ("ancount: %d\n", header->ancount);
    printf ("nscount: %d\n", header->nscount);
    printf ("arcount: %d\n", header->arcount);

}

