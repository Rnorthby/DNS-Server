//
// Created by robert on 2/5/21.
//

#ifndef COMP30023_2021_PROJECT_2_PHASE1_H
#define COMP30023_2021_PROJECT_2_PHASE1_H


void print_head(dns_header_type * head);
void print_question(question_type * question);
void print_response(response_type * response);

void free_header(dns_header_type * header);
void free_question(question_type * question);
void free_response(response_type * response);

#endif //COMP30023_2021_PROJECT_2_PHASE1_H
