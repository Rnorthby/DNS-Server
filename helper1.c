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


#include "helper1.h"

// function is called with is_response as 0 when from the client and 1 when from dns.
dns_message * read_request(char * buffer, int read_size, uint8_t is_response){
    // copy the message length.
    dns_message * message;
    message = (dns_message *) malloc(sizeof(dns_message));
    int byte_index = 0;
    memcpy(message, buffer + byte_index, 2);
    byte_index += 2;

    message->message_length = ntohs(message->message_length);

    // read the header from buffer
    message->header = read_header_buffer(buffer, &byte_index);

    // read the question from the buffer.
    message->question = read_question_buffer(buffer, &byte_index);

    // is the packet not of type AAAA
    if(message->question->q_type != 28){
        // print the question to the log file.
        fprint_question(message);
        return NULL;
    }
    // if the packet is from the dns server
    if(is_response){

        if(!message->header->an_count){
            printf("No Responses in this response Packet\n");
            return NULL;
        }
        // read the response
        message->response = read_response_buffer(buffer, &byte_index);
    }
    return message;

}

// reads the header information from a dns message buffer.
dns_header_type * read_header_buffer(char * buffer, int * byte_index){
    dns_header_type * header = (dns_header_type *) malloc(sizeof(dns_header_type));
    // id
    memcpy(header, buffer + *byte_index, 2);
    *byte_index += 2;
    header->id = ntohs(header->id);

    // read params.
    uint8_t temp;

    memcpy(&temp, buffer + *byte_index, 1);
    *byte_index += 1;
    // bit wise to seperate byte 5 and 6 into information.
    header->rd = (temp >> 0) & 1;
    header->tc = (temp >> 1) & 1;
    header->aa = (temp >> 2) & 1;
    header->op_code = (temp >> 3) & 7;
    header->qr = (temp >> 7) & 1;

    memcpy(&temp, buffer + *byte_index, 1);
    *byte_index += 1;

    header->r_code = (temp >> 0) & 15;
    header->z = (temp >> 4) & 7;
    header->ra = (temp >> 7) & 1;
    // read rest of header.
    memcpy(&header->qd_count, buffer + *byte_index, 8);
    *byte_index += 8;

    header->qd_count = ntohs(header->qd_count);
    header->an_count = ntohs(header->an_count);
    header->ns_count = ntohs(header->ns_count);
    header->ar_count = ntohs(header->ar_count);
    return header;
}

// reads the question from the dns buffer string.
question_type * read_question_buffer(char * buffer, int * byte_index){
    message_type *head = (message_type *) malloc(sizeof(message_type));
    message_type *current = head;
    message_type *previous = head;
    // have read the first 14 bytes on the message.
    // read the first length
    memcpy(current, buffer + *byte_index, 1);
    *byte_index += 1;
    // read the domain names while the next length != 0
    uint8_t i;
    while (current->length != 0) {
        current->word = (uint8_t *) malloc(sizeof (uint8_t) * (current->length + 1));
        for(i = 0; i < current->length; i++){
            memcpy(&(current->word[i]), buffer + *byte_index, 1);
            *byte_index += 1;
        }
        current->word[current->length] = '\0';
        current->next = (message_type *) malloc(sizeof(message_type));
        previous = current;
        current = current->next;
        // copy the next name length
        memcpy(current, buffer + *byte_index, 1);
        *byte_index += 1;
    }
    free(current);
    previous->next = NULL;


    question_type * question = (question_type *) malloc(sizeof(message_type));
    // create the question data structure and return.
    question->head = head;

    memcpy(&question->q_type, buffer + *byte_index, 2);
    *byte_index += 2;
    memcpy(&question->q_class, buffer + *byte_index, 2);
    *byte_index += 2;
    question->q_class = htons(question->q_class);
    question->q_type = htons(question->q_type);
    return question;
}

// reads the response of a dns packet buffer.
 response_type * read_response_buffer(char * buffer, int * byte_index){
    // create the data structure.
    response_type * response = (response_type *) malloc(sizeof(response_type));
    memcpy(&response->name, buffer + *byte_index, 2);

    *byte_index += 2;
    // 1111111111111 == 8191
    response->name = htons(response->name) & 8191;
    // copy the attributes to the data structure.
    memcpy(&response->q_type, buffer + *byte_index, 2);
    * byte_index += 2;
    memcpy(&response->q_class, buffer + *byte_index, 2);
    * byte_index += 2;
    memcpy(&response->ttl, buffer + *byte_index, 4);
    * byte_index += 4;
    memcpy(&response->rd, buffer + *byte_index, 2);
    * byte_index += 2;
    response->q_type = htons(response->q_type);
    response->q_class = htons(response->q_class);
    response->ttl = htonl(response->ttl);
    response->rd = htons(response->rd);

    // read the IP
    char ip[INET6_ADDRSTRLEN];
    memcpy(ip, buffer + *byte_index, INET6_ADDRSTRLEN);
    inet_ntop(AF_INET6, ip, response->addr, INET6_ADDRSTRLEN);
    *byte_index += INET6_ADDRSTRLEN;
    // print the ip to stdout for testing.
    printf("IP: %s\n", response->addr);

    // not going to read the AR or the rest of message.
    return response;
}

// free the request
void free_request(dns_message * request, uint8_t is_response){
    dns_free_header(request->header);
    dns_free_question(request->question);
    if(is_response){
        dns_free_response(request->response);
    }
    free(request);
}

// free the header.
void dns_free_header(dns_header_type * header){
    free(header);
}

// free the question
void dns_free_question(question_type * question){
    message_type * current = question->head;
    message_type * previous;
    while(current != NULL){
        free(current->word);
        previous = current;
        current = current->next;
        free(previous);
    }
    free(question);
}

// free the response.
void dns_free_response(response_type * response){
    if(response != NULL){
        free(response);
    }
}


// prints the time requests domain.name to the log file.
void fprint_question(dns_message * message){
    FILE * fptr;
    fptr = fopen("dns_svr.log", "a");
    // print the time.
    time_t sec;
    sec = time(NULL);
    struct tm * local;
    local = localtime(&sec);
    fprintf(fptr, "%d-%.2d-%.2dT%.2d:%.2d:%.2d+0000 ", local->tm_year + 1900, local->tm_mon, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);

    fprintf(fptr, "requested ");
    message_type * current = message->question->head;
    uint8_t i;
    // print the domain to the log file.
    while(current != NULL){
        for(i = 0; i < current->length; i++){
            fprintf(fptr, "%c", current->word[i]);
        }
        if(current->next != NULL) fprintf(fptr, ".");
        current = current->next;
    }
    fprintf(fptr, "\n");
    fclose(fptr);
}

// print the response to the file.
void fprint_response(dns_message * message){
    FILE * fptr;
    fptr = fopen("dns_svr.log", "a");
    // print the time.
    time_t sec;
    sec = time(NULL);
    struct tm * local;
    local = localtime(&sec);
    fprintf(fptr, "%d-%.2d-%.2dT%.2d:%.2d:%.2d+0000 ", local->tm_year + 1900, local->tm_mon, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
    message_type * current = message->question->head;
    uint8_t i;
    // print the requested domain.
    while(current != NULL){
        for(i = 0; i < current->length; i++){
            fprintf(fptr, "%c", current->word[i]);
        }
        if(current->next != NULL) fprintf(fptr, ".");
        current = current->next;
    }
    // print the location IP address.
    fprintf(fptr, " is at ");
    fprintf(fptr, "%s\n", message->response->addr);
    fclose(fptr);
}

// print no implementation to the log file.
void fprint_no_implementation(){
    FILE * fptr;
    fptr = fopen("dns_svr.log", "a");
    time_t sec;
    sec = time(NULL);
    struct tm * local;
    local = localtime(&sec);
    local = localtime(&sec);
    fprintf(fptr, "%d-%.2d-%.2dT%.2d:%.2d:%.2d+0000 ", local->tm_year + 1900, local->tm_mon, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
    fprintf(fptr, "unimplemented request\n");
    fclose(fptr);
}

// function updates the buffer for a no implementation response with updated r code.
void update_r_code(char * buffer){
    u_int8_t temp;
    // bit wise operations to fit the buffer for this response.
    memcpy(&temp, buffer + 4, 1);
    temp = temp | 128;
    temp = temp & 254;
    memcpy(buffer + 4, &temp, 1);
    memcpy(&temp, buffer + 5, 1);
    temp = temp | 4;
    memcpy(buffer + 5, &temp, 1);
    u_int16_t qd_count;
    memcpy(&qd_count, buffer + 6, 2);
    qd_count = htons(qd_count);
}
