#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "dns.h"


/*
int main(int argc, char* argv[]) {
    uint8_t is_response;
    if(argv[1][0] == 'r'){
        is_response = 1;
    }
    else{
        is_response = 0;
    }
    printf("%d\n", is_response);
    read_from_client(STDIN_FILENO);
}
*/

int read_from_client(int file){
    uint8_t is_response = 1;
    dns_message *message = (dns_message *) malloc(sizeof(dns_message));
    message->response = NULL;
    read(file, message, 2);

    message->message_length = htons(message->message_length);

    message->header = read_header(file);
    if(is_response && !message->header->an_count){
        printf("NONE\n");
        exit(EXIT_SUCCESS);
    }
    message->question = read_question(file);

    print_head(message->header);
    print_question(message->question);


    FILE * fptr;
    fptr = fopen("dns_svr.log", "w");

    time_t sec;
    sec = time(NULL);
    struct tm * local;
    local = localtime(&sec);
    printf("Hello\n");
    printf("%d\n", local->tm_sec);
    fprintf(fptr, "%d-%.2d-%.2dT%.2d:%.2d:%.2d+0000 ", local->tm_year + 1900, local->tm_mon, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);

    if(message->question->q_type != 28){
        fprintf(fptr, "unimplemented request");

    }
    else if(is_response){
        // handle the response variables
        message->response = read_response(file);
        print_response(message->response);
        message_type * current = message->question->head;
        uint8_t i;
        while(current != NULL){
            for(i = 0; i < current->length; i++){
                fprintf(fptr, "%c", current->word[i]);
            }
            if(current->next != NULL) fprintf(fptr, ".");
            current = current->next;
        }
        fprintf(fptr, " is at ");
        response_type * response = message->response;
        fprintf(fptr, "%x", response->ip[0]);
        int prev = 1;
        int prev2 = 1;
        for(i = 1; i < response->rd / 2; i++){
            if(prev2){
                fprintf(fptr, ":");
            }
            if(response->ip[i]){
                fprintf(fptr, "%x", response->ip[i]);
                prev = 1;
                prev2 =1;
            }
            else if(prev){
                prev = 0;
            }
            else{
                prev2 = 0;
            }
        }

        free_response(message->response);
    }
    else{
        fprintf(fptr, "requested ");
        message_type * current = message->question->head;
        uint8_t i;
        while(current != NULL){
            for(i = 0; i < current->length; i++){
                fprintf(fptr, "%c", current->word[i]);
            }
            if(current->next != NULL) fprintf(fptr, ".");
            current = current->next;
        }


    }
    fclose(fptr);
    free_header(message->header);

    free_question(message->question);

    free(message);
    return 0;
}


dns_header_type * read_header(int file){
    dns_header_type * header = (dns_header_type *) malloc(sizeof(dns_header_type));
    read(file, header, 2);
    // bit wise reading.
    uint8_t temp;
    read(file, &temp, 1);
    header->rd = (temp >> 0) & 1;
    header->tc = (temp >> 1) & 1;
    header->aa = (temp >> 2) & 1;
    header->op_code = (temp >> 3) & 7;
    header->qr = (temp >> 7) & 1;

    read(file, &temp, 1);
    header->r_code = (temp >> 0) & 15;
    header->z = (temp >> 4) & 7;
    header->ra = (temp >> 7) & 1;

    read(file, &(header->qd_count), 8);
    header->id = htons(header->id);
    header->qd_count = htons(header->qd_count);
    header->an_count = htons(header->an_count);
    header->ns_count = htons(header->ns_count);
    header->ar_count = htons(header->ar_count);

    return header;
}

question_type * read_question(int file){
    message_type *head = (message_type *) malloc(sizeof(message_type));
    message_type *current = head;
    message_type *previous = head;
    read(file, current, 1);
    uint8_t i;
    printf("%d\n", current->length);
    while (current->length != 0) {
        current->word = (uint8_t *) malloc(sizeof(uint8_t) * (current->length + 1));
        for(i = 0; i < current->length; i++){
            read(file, &(current->word[i]), 1);
        }
        //read(0, &(current->word), current->length);
        current->word[current->length] = '\0';
        printf("%s\n", current->word);
        current->next = (message_type *) malloc(sizeof(message_type));
        previous = current;
        current = current->next;
        read(file, current, 1);
    }
    free(current);
    previous->next = NULL;

    question_type * question = (question_type *) malloc(sizeof(message_type));

    question->head = head;
    read(file, &question->q_type, 2);
    read(file, &question->q_class, 2);
    question->q_class = htons(question->q_class);
    question->q_type = htons(question->q_type);
    return question;
}

response_type * read_response(int file){
    response_type * response = (response_type *) malloc(sizeof(response_type));

    read(file, &response->name, 2);
    // 1111111111111 == 8191
    response->name = htons(response->name) & 8191;

    read(file, &response->q_type, 2);
    read(file, &response->q_class, 2);
    read(file, &response->ttl, 4);
    read(file, &response->rd, 2);
    response->q_type = htons(response->q_type);
    response->q_class = htons(response->q_class);
    response->ttl = htonl(response->ttl);
    response->rd = htons(response->rd);

    response->ip = (uint16_t *) malloc(sizeof(uint32_t) * response->rd);
    u_int8_t i;
    for(i = 0; i < response->rd / 2; i++){
        read(file, &response->ip[i], 2);
        response->ip[i] = htons(response->ip[i]);
    }
    // not going to read the AR.
    return response;


}

void free_header(dns_header_type * header){
    free(header);
}

void free_question(question_type * question){
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

void free_response(response_type * response){
    if(response != NULL){
        free(response->ip);
        free(response);
    }
}



void print_head(dns_header_type * head){
    printf("-----MESSAGE HEADER-----\n\n");
    printf("ID: %d\n", head->id);
    printf("QR: %x\n", head->qr);
    printf("Op Code: %x\n", head->op_code);
    printf("AA: %d\n", head->aa);
    printf("TC: %d\n", head->tc);
    printf("RD: %d\n", head->rd);
    printf("RA: %d\n", head->ra);
    printf("Z: %d\n", head->z);
    printf("R Code: %d\n", head->r_code);
    printf("QD Count: %d\n", head->qd_count);
    printf("AN Count: %d\n", head->an_count);
    printf("NS Count: %d\n", head->ns_count);
    printf("AR Count: %d\n", head->ar_count);
}


void print_question(question_type * question){
    printf("-----MESSAGE QUESTION-----\n\n");
    message_type * current = question->head;
    uint8_t i;
    while(current != NULL){
        for(i = 0; i < current->length; i++){
            printf("%c", current->word[i]);
        }
        printf("\n");
        current = current->next;
    }
    printf("q class: %.4x \n", question->q_class);
    printf("q type: %.4x \n", question->q_type);

    return;
}


void print_response(response_type * response){
    printf("-----MESSAGE RESPONSE-----\n\n");

    printf("name: %d\n", response->name);
    printf("q class: %.4x\n", response->q_class);
    printf("q type: %.4x\n", response->q_type);

    printf("TTL: %x\n", response->ttl);
    printf("rd: %d\n", response->rd);
    uint8_t i;
    printf("---------------IPv6---------------\n");
    printf("%x", response->ip[0]);
    for(i = 1; i < response->rd / 2; i++){
        printf(":");
        if(response->ip[i]) printf("%x", response->ip[i]);
    }
    printf("\n");
}
