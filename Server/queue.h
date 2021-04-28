//
// Created by vlad on 1/5/21.
//

#ifndef SERVER_QUEUE_H
#define SERVER_QUEUE_H

struct node {
    struct node * next;
    int * client_socket;
};

typedef struct node node_t;

void push (int * client_socket);
int * pop ();

#endif //SERVER_QUEUE_H
