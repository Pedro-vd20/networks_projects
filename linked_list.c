#include <stddef.h>
#include <stdlib.h>

#include "linked_list.h"
#include "packet.h"

// make a new linked list
linked_list make_list(tcp_packet* packet) {
    linked_list ls;
    
    ls.head = malloc(sizeof(struct node));
    ls.tail = ls.head;

    ls.head->p = packet;

    ls.head->next = NULL;

    return ls;
}

// add to the end of the list
// bool add_node(linked_list* ls, tcp_packet packet) {
    // return true;
// }