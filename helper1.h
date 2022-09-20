

#include <sys/types.h>
#ifndef COMP30023_2021_PROJECT_2_HELPER_H
#define COMP30023_2021_PROJECT_2_HELPER_H
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
    char addr[INET6_ADDRSTRLEN];

} response_type;


// dns message DS
typedef struct {
    u_int16_t message_length;
    dns_header_type * header;
    question_type * question;
    response_type * response;
} dns_message;



dns_message * read_request(char * buffer, int read_size, uint8_t is_response);

dns_header_type * read_header_buffer(char * buffer, int * byte_index);
question_type * read_question_buffer(char * buffer, int * byte_index);
response_type * read_response_buffer(char * buffer, int * byte_index);


void free_request(dns_message * request, u_int8_t is_response);

void dns_free_header(dns_header_type * header);
void dns_free_question(question_type * question);
void dns_free_response(response_type * response);

void fprint_question(dns_message * message);
void fprint_response(dns_message * message);
void fprint_no_implementation();

void update_r_code(char * buffer);


#endif //COMP30023_2021_PROJECT_2_HELPER_H
