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
    remove_node(ls, ls->size);
    return 0;
}

tcp_packet *get_head(linked_list* TCP_window){
    // if(TCP_window->head == TCP_window->tail){
    //     free(TCP_window->head);
    //     TCP_window->head = NULL;
    //     return
    // }
    return TCP_window-> head -> p;
}


int remove_node(linked_list* TCP_window, int num_nodes) {

    for(int i = 0; i < num_nodes; i++ ){
        if(is_empty(TCP_window)){
            return -1;
        }
        struct node *temp_pointer = TCP_window->head;
        TCP_window->head = TCP_window->head->next;
        free(temp_pointer);
        TCP_window->size--;
    }
    return 0;
}