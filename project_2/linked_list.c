#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "linked_list.h"

int init(linked_list* ls) {
    if(ls == NULL) {
        return -1;
    }
    
    ls->head = NULL;
    ls->tail = NULL;
    ls->size = 0;

    return 0;
}

// add to the end of the list
int add_node(linked_list* ls, char* path) {
    if(is_empty(ls)) {
        ls->head = malloc(sizeof(struct node));
        ls->head->p = path;
        ls->tail = ls->head;
        ls->head->prev = NULL;
    }
    else {
        struct node* newNode = malloc(sizeof(struct node));
        newNode->p = path;
        newNode->prev = ls->tail;
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
    while(ls->size > 0) {
        remove_node(ls);
    }
    return 0;
}

int remove_node(linked_list* ls) {    
    if(is_empty(ls)) {
        return -1;
    }

    struct node* temp_pointer = ls->tail;
    ls->tail = temp_pointer->prev;

    free(temp_pointer->p);
    free(temp_pointer);
    ls->size--;
    if(!is_empty(ls)) {
        ls->tail->next = NULL;
    }

    return 0;
}

char* get_path(linked_list* ls) {
    char* path = malloc(1024);
    bzero(path, sizeof(path));
    
    if(ls->size == 0) {
        strcpy(path, " ");
        return path;
    }
    
    

    struct node* curr = ls->head;
    for(int i = 0; i < ls->size; ++i) {
        strcat(path, curr->p);
        strcat(path, "/");

        curr = curr->next;
    }

    return path;
}

// debug methods
void print(linked_list* ls) {
    printf("List with %d items\n", ls->size);

    struct node* curr = ls->head;
    for(int i = 0; i<ls->size; ++i) {
        char* path = curr->p;
        printf("%s/", path);

        curr = curr->next;
    }

    printf("\n");
}

int parse_dir(linked_list* path, char* new_path, int min_len) {
    // break down path by /
    char* token = strtok(new_path, "/");

    do {
        if(strcmp(token, "..") == 0) {
            if(path->size > min_len) {
                remove_node(path);
            }
            else {
                return -1;
            }
        }
        else {
            char* temp = malloc(64);
            bzero(temp, sizeof(temp));
            strcpy(temp, token);
            add_node(path, temp);
        }

        // get new token
        token = strtok(NULL, "/");
    } while(token != NULL);
    
    return 0;
}