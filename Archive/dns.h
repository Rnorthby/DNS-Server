//
// Created by robert on 2/5/21.
//

#ifndef COMP30023_2021_PROJECT_2_DNS_H
#define COMP30023_2021_PROJECT_2_DNS_H
#include <sys/types.h>

// header DS
typedef struct {
    uint16_t id;
    uint8_t qr:1;
    uint8_t op_code:4;
    uint8_t aa:1;
    uint8_t tc:1;
    uint8_t rd:1;
    uint8_t ra:1;
    uint8_t z:3;
    uint8_t r_code:4;
    uint16_t qd_count;
    uint16_t an_count;
    uint16_t ns_count;
    uint16_t ar_count;
} dns_header_type;


// the question DS
typedef struct message_type message_type;

struct message_type {
    u_int8_t  length;
    u_int8_t * word;
    message_type * next;
};

typedef struct {
    message_type * head;
    u_int16_t q_type;
    u_int16_t q_class;
} question_type;

// The Response DS
typedef struct {
    u_int16_t name;
    u_int16_t q_type;
    u_int16_t q_class;
    u_int32_t ttl;
    u_int16_t rd;
    u_int16_t * ip;

} response_type;


// dns message DS
typedef struct {
    u_int16_t message_length;
    dns_header_type * header;
    question_type * question;
    response_type * response;
} dns_message;



// functions
dns_header_type * read_header(int file);
question_type * read_question(int file);
response_type * read_response(int file);

void print_head(dns_header_type * head);
void print_question(question_type * question);
void print_response(response_type * response);

void free_header(dns_header_type * header);
void free_question(question_type * question);
void free_response(response_type * response);


int read_from_client(int file);

#endif //COMP30023_2021_PROJECT_2_DNS_H
