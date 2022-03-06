#ifndef LINKED_LIST
#define LINKED_LIST

#include "packet.h"

typedef struct {
    tcp_packet p;
    node* next;    
} node;

typedef struct {
    node* head;
    node* tail;
} linked_list;

linked_list make_list(tcp_packet);
// add to the end of the list
bool add_node(linked_list*, tcp_packet);
tcp_packet get_head(linked_list*);
// Remove int items starting from first
bool remove_node(linked_list*, int);


#endif