#include <stddef.h>
#include <stdlib.h>

#include "linked_list.h"
#include "packet.h"

// add to the end of the list
int add_node(linked_list* ls, tcp_packet* packet) {
    if(is_empty(ls)) {
        ls->head = malloc(sizeof(struct node));
        ls->head->p = packet;
        ls->tail = ls->head;
    }
    else {
        struct node* newNode = malloc(sizeof(struct node));
        newNode->p = packet;
        ls->tail->next = newNode;
        ls->tail = newNode;
    }
    ls->tail->next = NULL;
    (ls->size)++;

    return 0;
}

int is_empty(linked_list* ls) {
    return ls->size == 0;
}

// free all memory 
int delete_list(linked_list* ls) {
    while(!is_empty(ls)) {
        remove_node(ls, 1);
    }
    return 0;
}