#ifndef LINKED_LIST
#define LINKED_LIST

struct node {
    char* p;
    struct node* next;   
    struct node* prev; 
};

typedef struct {
    struct node* head;
    struct node* tail;
    int size;
} linked_list;

// initialize values for list
int init(linked_list*);

// add to the end of the list
int add_node(linked_list*, char*);

// combines all nodes into a file path
char* get_path(linked_list*);

// Remove from back
int remove_node(linked_list*);

// check if empty
int is_empty(linked_list*);

// free all memory 
int delete_list(linked_list*);

// send packets
int change_dir(linked_list*, char*);

// Debug methods
void print(linked_list*);


#endif