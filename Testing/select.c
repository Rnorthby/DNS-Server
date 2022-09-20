// select-server-1.2.c -- a cheezy multiperson chat server (modified)

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

#include <assert.h>
#include <time.h>
#include "dns.h"


// 127.0.0.53 on port 53.

int dnsSocket();


int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s ip port\n", argv[0]);
        return 0;
    }




    struct addrinfo hints, *res;
    // create address we're going to listen on (with given port number)
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // for bind, listen, accept
    // node (NULL means any interface), service (port), hints, res
    int s = getaddrinfo(argv[1], argv[2], &hints, &res);
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

    // listen on the socket
    listen(sockfd, 5);

    // initialise an active file descriptors set
    fd_set masterfds;
    FD_ZERO(&masterfds);
    FD_SET(sockfd, &masterfds);
    // record the maximum socket number
    int maxfd = sockfd;
    uint8_t response_waiting[FD_SETSIZE];
    int i;
    for(i = 0; i < FD_SETSIZE; i++){
        response_waiting[i] = 0;
    }




    int dns_socket = dnsSocket();
    printf("here\n");


    while (1) {
        // monitor file descriptors
        fd_set readfds = masterfds;
        // read(socket, DNS_MEssage_header, sizeof(message_header));

        if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // loop all possible descriptor
        for (i = 0; i <= maxfd; ++i)
            // determine if the current file descriptor is active
            if (FD_ISSET(i, &readfds)) {
                // create new socket if there is new incoming connection request
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
                    /*if(!response_waiting[i]) {
                        // read the request store the request id as response_waiting value.
                        int status = read_from_client(i);
                    }*/
                    char buffer[255];
                    int size = read(i, buffer, 255);

                    // read the message and cache.

                    write(dns_socket, buffer, size);

                    // read is a blocking.
                    size = read(dns_socket, buffer, 255);

                    // store in the cache.

                    write(i, buffer, 255);
                    FD_CLR(i, &masterfds);

                    /*
                    dns_message * message = (dns_message *) malloc(sizeof(dns_message));

                    read(i, message, 2);
                    message->message_length = htons(message->message_length);
                    printf("Message Length: %d\n", message->message_length);
                    message->header = read_header(i);
                    print_head(message->header);
                    message->question = read_question(i);
                    print_question(message->question);
                    write(i, message, sizeof(dns_message));
                    */

                    // search cache.


                    // send to dns


                    // recieve the dns message

                    // save to cache
                    // forward the response.


                    /*
                    char buff[256];
                    int n = read(i, buff, 256);
                    if (n <= 0) {
                        if (n < 0)
                            perror("read");
                        else
                            printf("socket %d close the connection\n", i);
                        close(i);
                        FD_CLR(i, &masterfds);
                    }
                        // write back to the client
                    else if (write(i, buff, n) < 0) {


                        perror("write");
                        close(i);
                        FD_CLR(i, &masterfds);
                    }
                     */


                }
            }
    }
    return 0;
}


int dnsSocket(){
    int sockfd, n, s;
    struct addrinfo hints, *servinfo, *rp;
    char buffer[256];


    // Create address
    char * ip = "128.250.201.5";
    char * port = "53";

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
    if (rp == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(servinfo);
    return sockfd;
}