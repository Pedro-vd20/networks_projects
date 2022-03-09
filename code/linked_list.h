#ifndef LINKED_LIST
#define LINKED_LIST

#include "packet.h"

struct node {
    tcp_packet* p;
    struct node* next;    
};

typedef struct {
    struct node* head;
    struct node* tail;
    int size;
} linked_list;

// add to the end of the list
int add_node(linked_list*, tcp_packet*);
tcp_packet* get_head(linked_list*);
// Remove int items starting from first
int remove_node(linked_list*, int);
int is_empty(linked_list*);
// free all memory 
int delete_list(linked_list*);

// Debug methods
void print(linked_list*);


#endif