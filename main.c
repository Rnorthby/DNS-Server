#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>

#include <assert.h>
#include <time.h>
#include "helper1.h"
#include "cache.h"
#include "non_blocking.h"

#define CACHE

#define LOG_FILE "dns_svr.log"


// 127.0.0.53 on port 53.

int dnsSocket(char * ip, char * port);
void * run_dns_lookup(void * arg);
int home_socket();

cache_5_t * cache;
pthread_mutex_t  cache_lock;
pthread_mutex_t write_lock;

int main(int argc, char* argv[]) {
    printf("Start");
    // create socket
    int sockfd = home_socket();

    // listen on the socket
    listen(sockfd, 15);

    // initialise an active file descriptors set
    fd_set masterfds;
    FD_ZERO(&masterfds);
    FD_SET(sockfd, &masterfds);

    // record the maximum socket number
    int maxfd = sockfd;

    // clear the dns_svr.log file.
    FILE * fptr;
    fptr = fopen("dns_svr.log", "w");
    fclose(fptr);

    cache = init_cache();

    pthread_t tid;
    int i;

    while (1) {
        // the read files
        fd_set readfds = masterfds;

        if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // loop all possible descriptor
        for (i = 0; i <= maxfd; ++i)
            // determine if the current file descriptor is active
            if (FD_ISSET(i, &readfds)) {
                // create new socket if there is new incoming connection request
                // reused the below standard select code from the tut.
                if (i == sockfd) {
                    struct sockaddr_in cliaddr;
                    socklen_t clilen = sizeof(cliaddr);
                    int newsockfd =
                            accept(sockfd, (struct sockaddr*)&cliaddr, &clilen);
                    if (newsockfd < 0)
                        perror("accept");
                    else {
                        // add the socket to the set
                        FD_SET(newsockfd, &masterfds);
                        // update the maximum tracker
                        if (newsockfd > maxfd)
                            maxfd = newsockfd;
                        // print out the IP and the socket number
                        char ip[INET_ADDRSTRLEN];
                        printf("new connection from %s on socket %d\n",
                                // convert to human readable string
                               inet_ntop(cliaddr.sin_family, &cliaddr.sin_addr,
                                         ip, INET_ADDRSTRLEN),
                               newsockfd);
                    }
                }
                    // a message is sent from the client
                else {
                    sockets_t sockets;
                    sockets.dns_socket = dnsSocket(argv[1], argv[2]);
                    sockets.client_socket = i;
                    // run the p thread with the two sockets as an arg.
                    pthread_create(&tid, 0, run_dns_lookup, (void *) &sockets);
                    // store in the cache.
                    FD_CLR(i, &masterfds);
                }
            }
    }
    return 0;
}

int home_socket(){
    char * port = "8053";

    struct addrinfo hints, *res;
    // create address we're going to listen on (with given port number)
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;

    int s = getaddrinfo(NULL, port, &hints, &res);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    // create socket
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // reuse the socket if possible
    int const reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // bind address to socket
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}




int dnsSocket(char * ip, char * port){
    int sockfd, s;
    struct addrinfo hints, *servinfo, *rp;


    // Create address
    //char * ip = "128.250.201.5";
    //char * port = "53";

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Get addrinfo of server. From man page:
    // The getaddrinfo() function combines the functionality provided by the
    // gethostbyname(3) and getservbyname(3) functions into a single interface
    s = getaddrinfo(ip, port, &hints, &servinfo);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    // Connect to first valid result
    // Why are there multiple results? see man page (search 'several reasons')
    // How to search? enter /, then text to search for, press n/N to navigate
    for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1)
            continue;
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break; // success

        close(sockfd);
    }
    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt");exit(1);
    }
    if (rp == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(servinfo);
    return sockfd;
}




// two locks added. One to avoid concurrent changes to the cache and one to stop concurrent writes to the log file.
void * run_dns_lookup(void * arg){
    // buffer to read into.
    char buffer[256];
    // get the args. (Client socket and dns server socket)
    sockets_t sockets = *((sockets_t *)arg);
    int dns_socket = sockets.dns_socket;
    int i = sockets.client_socket;

    // read the message and pharse the message length
    int total_read_size = 0;
    int size = read(i, buffer, 255);
    total_read_size += size;
    int message_size;
    memcpy(&message_size, buffer, 2);
    message_size = htons(message_size);
    printf("size: %d\nTo Read: %d\n", size, message_size);
    // make sure that the whole message has been read.
    while(message_size + 2 != total_read_size){
        size = read(i, buffer + total_read_size, 255 - total_read_size);
        total_read_size += size;
        buffer[total_read_size] = '\0';
    }

    // read the message request.
    pthread_mutex_lock(&write_lock);
    dns_message * read_message = read_request(buffer, size, 0);
    pthread_mutex_unlock(&write_lock);

    // if it is not a AAAA packet type.
    if(read_message == NULL){
        // print the no implementation to the log file.
        pthread_mutex_lock(&write_lock);
        fprint_no_implementation();
        pthread_mutex_unlock(&write_lock);
        // update the buffer for the response.
        update_r_code(buffer);
        // send back to the client.
        write(i, buffer, 255);
    }
    else{
        // print the question to the log file.
        pthread_mutex_lock(&write_lock);
        fprint_question(read_message);
        pthread_mutex_unlock(&write_lock);

        // allow 1 thread to access the cache and search for the cached answer.
        pthread_mutex_lock(&cache_lock);
        char * cache_return = get_cache_response(buffer, cache);
        pthread_mutex_unlock(&cache_lock);

        if(cache_return){
            // is in the cache.
            printf("Found in the cache\n");
            write(i, cache_return, 255);
        }
        else{
            // free the request.
            free_request(read_message, 0);
            write(dns_socket, buffer, total_read_size);
            // read is a blocking.
            // make sure that it is fully read.
            total_read_size = 0;
            size = read(dns_socket, buffer, 255);
            memcpy(&message_size, buffer, 2);
            message_size = htons(message_size);
            printf("size: %d\nTo Read: %d\n", size, message_size);

            total_read_size += size;
            // make sure the whole request is read.
            while(message_size + 2 != total_read_size){
                size = read(dns_socket, buffer + total_read_size, 255 - total_read_size);
                total_read_size += size;
            }

            if(size > 0){
                // read the new request from the buffer.
                pthread_mutex_lock(&write_lock);
                dns_message * response_message = read_request(buffer, total_read_size, 1);
                pthread_mutex_unlock(&write_lock);
                // if a response is found and it is of type 1c.
                // look at the r code.
                if(response_message){
                    // add the new response to the buffer.
                    pthread_mutex_lock(&cache_lock);
                    insert_response_cache(buffer, cache, response_message->response->ttl, response_message->response->addr);
                    pthread_mutex_unlock(&cache_lock);
                    // see if they respond with AAAA record and print to log.
                    if(response_message->response->q_type == 28){
                        fprint_response(response_message);
                    }
                    // free request.
                    if(!read_message){
                        free_request(response_message, 1);

                    }
                    // write the buffer back to the client.
                    write(i, buffer, total_read_size);
                }
            }

        }
    }
    return NULL;
}
