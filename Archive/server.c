//
// Created by robert on 8/5/21.
//


// have 1 select pool.
// read the client dns request. test if it is from client.
// place the client connection to a hash table to match the incoming answer to the client
// map ID's sent to the server and the client through hash table or other structure.
// write to the home server for a response.
// when the response arrives, sendto() the answer to the client.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <unistd.h>

#include <assert.h>
#include <time.h>
#include "dns.h"
#include "dns2.h"

#define PORT "53"
#define HOME_SERVER 127.0.0.53


int main(int argc, char* argv[]) {
    int server_socket = setup_socket()
}