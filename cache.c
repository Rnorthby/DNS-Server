//
// Created by robert on 11/5/21.
//
#include <stdio.h>
#include <stdlib.h>
# include <string.h>
#include <time.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "cache.h"

// start the cache data structure up
cache_5_t * init_cache(){
    cache_5_t * cache = (cache_5_t *) malloc(sizeof(cache_5_t));
    cache->next_index = 0;
    int i;
    for(i =0; i < CACHE_SIZE; i ++){
        cache->has_data[i] = 0;
        cache->death_seconds[i] = UINT32_MAX;
    }
    return cache;
}

// return the updated buffer reponse if found or null.
char * get_cache_response(char * buffer, cache_5_t * cache){
    // search the cache.
    int cache_index = search_cache(buffer, cache);
    if(cache_index < 0){
        return NULL;
    }
    // create a response from the cache if a response is matched.
    char * output = create_response_from_cache(buffer, cache->buffers[cache_index], cache->death_seconds[cache_index]);
    // fprintf the log for when the item leaves the cache.
    print_cache_match(cache, cache_index);
    print_returned_request(cache->buffers[cache_index], cache->ip[cache_index]);
    return output;
}

// seach the cache for a buffer.
int search_cache(char * buffer, cache_5_t * cache){
    short i;
    for(i = 0; i < CACHE_SIZE; i++){
        if(!cache->has_data[i]){
            continue;
        }
        else if(cache_item_match(buffer, cache->buffers[i])){
            // look if it has expired.
            if(cache->death_seconds[i] >= time(NULL)){
                return i;
            }
        }
    }
    return -1;
}

// see if two questions are the same. returns 1 if that is true.
int cache_item_match(char * message_buffer, char * question_buffer){
    int current_index = MESSAGE_INDEX;
    while(message_buffer[current_index] == question_buffer[current_index]){
        current_index++;
        if(message_buffer[current_index] == 0 && question_buffer[current_index] == 0){
            return 1;
        }
    }
    return 0;
}


void print_cache_match(cache_5_t * cache, int cache_index){
    // print eg 2021-04-26T01:03:36+0000 c0.comp30023 expires at 2021-04-27T01:03:36+0000
    FILE * fptr;
    fptr = fopen("dns_svr.log", "a");
    printf("FOUND AND PRINTING\n");
    // print current time.
    time_t sec;
    sec = time(NULL);
    struct tm * local;
    local = localtime(&sec);
    fprintf(fptr, "%d-%.2d-%.2dT%.2d:%.2d:%.2d+0000 ", local->tm_year + 1900, local->tm_mon, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);

    // print the domain name.
    int current_index = MESSAGE_INDEX;
    int length = cache->buffers[cache_index][current_index];
    current_index ++;
    int i;
    while(1){
        for(i = 0; i < length; i ++){
            fprintf(fptr, "%c", cache->buffers[cache_index][current_index]);
            current_index ++;
        }
        if(cache->buffers[cache_index][current_index] == 0){
            break;
        }
        else{
            length = cache->buffers[cache_index][current_index];
            current_index ++;
            fprintf(fptr, ".");
        }
    }

    // print the expiry.
    fprintf(fptr, " expires at ");
    struct tm * exp;
    exp = localtime(&cache->death_seconds[cache_index]);


    fprintf(fptr, "%d-%.2d-%.2dT%.2d:%.2d:%.2d+0000\n", exp->tm_year + 1900, exp->tm_mon, exp->tm_mday, exp->tm_hour, exp->tm_min, exp->tm_sec);
    fclose(fptr);
}

// print the request returned to the log.
void print_returned_request(char * buffer, char * ip){
    FILE * fptr;
    fptr = fopen("dns_svr.log", "a");
    // print the time.
    time_t sec;
    sec = time(NULL);
    struct tm * local;
    local = localtime(&sec);
    fprintf(fptr, "%d-%.2d-%.2dT%.2d:%.2d:%.2d+0000 ", local->tm_year + 1900, local->tm_mon, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
    // print the domain name.
    int current_index = MESSAGE_INDEX;
    int length = buffer[current_index];
    current_index ++;
    int i;
    // print the domain.
    while(1){
        for(i = 0; i < length; i ++){
            fprintf(fptr, "%c", buffer[current_index]);
            current_index ++;
        }
        if(buffer[current_index] == 0){
            break;
        }
        else{
            length = buffer[current_index];
            current_index ++;
            fprintf(fptr, ".");
        }
    }
    fprintf(fptr, " is at ");
    fprintf(fptr, "%s\n", ip);

    fclose(fptr);
}


// create a response buffer to be sent back to the client skipping the dns server.
char * create_response_from_cache(char * buffer, char * question_buffer, u_int32_t ttl){
    char * output = (char *) malloc(sizeof(char) * 255);
    memcpy(output, question_buffer, 255);
    // changing the id's to the buffer.
    memcpy(output + 2, buffer + 2, 2);
    // update the ttl.
    int ttl_index = 14;
    // get up to the end of the answer assuming that 1 answer given.
    while(buffer[ttl_index] != 0){
        ttl_index++;
    }
    ttl_index += 13;
    u_int32_t diff =  ttl - (u_int32_t) time(NULL);
    if(diff > 0){
        diff= ntohs(diff);
    }
    else{
        diff = ntohs(0);
    }
    memcpy(output + ttl_index, &diff, 3);
    return output;
}

// insert the new reponse into the cache.
void insert_response_cache(char * buffer, cache_5_t * cache, u_int32_t ttl, char * ip){
    // replace by the most dead.
    int i;
    int cache_index = cache->next_index;

    // find the most dead response.
    u_int32_t min_time = UINT32_MAX;
    for(i = 0; i < CACHE_SIZE; i++){
        if(cache->death_seconds[i] - time(NULL) <= 0){
            if(cache->death_seconds[i] < min_time){
                min_time = cache->death_seconds[i];
                cache_index= i;
            }
        }
    }
    // insert at next_index;
    // fprintf the replacing names.
    memcpy(cache->buffers[cache_index], buffer, 255);
    cache->death_seconds[cache_index] = ttl + time(NULL);
    printf("Inserted in Cache: ttl: %u Index: %d\n\n", ttl, cache_index);

    if(!cache->has_data[cache_index]){
        cache->has_data[cache_index] = 1;
    }
    else{
        print_cache_replacement(buffer, cache->buffers[cache_index]);
    }
    cache->has_data[cache_index] = 1;
    memcpy(cache->ip[cache_index], ip, INET6_ADDRSTRLEN);
    cache->next_index = (cache->next_index + 1) % (CACHE_SIZE);
    return;
}


// print the cache replacement to the log file.
void print_cache_replacement(char * old_buffer, char * new_buffer){
    FILE * fptr;
    fptr = fopen("dns_svr.log", "a");
    // print the time.
    time_t sec;
    sec = time(NULL);
    struct tm * local;
    local = localtime(&sec);
    fprintf(fptr, "%d-%.2d-%.2dT%.2d:%.2d:%.2d+0000 ", local->tm_year + 1900, local->tm_mon, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);

    // print the old buffer;
    fprintf(fptr, "replacing ");
    int current_index = MESSAGE_INDEX;
    int length = old_buffer[current_index];
    current_index ++;
    int i;
    while(1){
        for (i = 0; i < length; i ++){
            fprintf(fptr, "%c", old_buffer[current_index]);
            current_index ++;
        }
        if(old_buffer[current_index] == 0){
            break;
        }
        else {
            length = old_buffer[current_index];
            current_index ++;
            fprintf(fptr, ".");
        }
    }

    // print the new buffer.
    fprintf(fptr, " by ");
    current_index = MESSAGE_INDEX;
    length = new_buffer[current_index];
    current_index ++;
    while(1){
        for (i = 0; i < length; i ++){
            fprintf(fptr, "%c", new_buffer[current_index]);
            current_index ++;
        }
        if(new_buffer[current_index] == 0){
            break;
        }
        else {
            length = new_buffer[current_index];
            current_index ++;
            fprintf(fptr, ".");
        }
    }
    fprintf(fptr, "\n");
    fclose(fptr);
}
