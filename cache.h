//
// Created by robert on 11/5/21.
//

#ifndef COMP30023_2021_PROJECT_2_CACHE_H
#define COMP30023_2021_PROJECT_2_CACHE_H

#include <time.h>
#include <string.h>
//#include "helper1.h"
// create the cache data structure for storing 5 items.

#define CACHE_SIZE 5
#define MESSAGE_INDEX 14

// indirection of the c time struct.


typedef struct {
    int next_index;
    char buffers[CACHE_SIZE][255];
    int ttl_index[CACHE_SIZE];
    char has_data[CACHE_SIZE];
    char ip[CACHE_SIZE][INET6_ADDRSTRLEN];
    time_t death_seconds[CACHE_SIZE];
} cache_5_t;


cache_5_t * init_cache();

char * get_cache_response(char * buffer, cache_5_t * cache);
int search_cache(char * buffer, cache_5_t * cache);
int cache_item_match(char * message_buffer, char * question_buffer);
char * create_response_from_cache(char * buffer, char * question_buffer, u_int32_t ttl);
void print_cache_match(cache_5_t * cache, int cache_index);


void insert_response_cache(char * buffer, cache_5_t * cache, u_int32_t ttl, char * ip);
void print_cache_replacement(char * old_buffer, char * new_buffer);
void print_returned_request(char * buffer, char * ip);

#endif //COMP30023_2021_PROJECT_2_CACHE_H
