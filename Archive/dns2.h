//
// Created by robert on 8/5/21.
//

#ifndef COMP30023_2021_PROJECT_2_DNS2_H
#define COMP30023_2021_PROJECT_2_DNS2_H


int setup_socket();
int accept_new_connection(int server);
void handle_connection(int i);

#endif //COMP30023_2021_PROJECT_2_DNS2_H
