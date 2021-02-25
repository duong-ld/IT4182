//
//  linkList.c
//  indexWord
//
//  Created by Lương Dương on 24/02/2021

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "constain.h"
#include "linkList.h"
 
node CreateNode(char* value, int line){
    node temp;
    temp = (node)malloc(sizeof(struct LinkedList));
    temp->next = NULL;
    strcpy(temp->data, value);
    temp->lines[0] = line;
    temp->idx = 0;
    return temp;
}
 
node AddTail(node head, char* value, int line){
    node temp,p;
    temp = CreateNode(value, line);
    if(head == NULL){
        head = temp;
    }
    else{
        p  = head;
        while(p->next != NULL){
            p = p->next;
        }
        p->next = temp;
    }
    return head;
}
node AddHead(node head, char* value, int line){
    node temp = CreateNode(value, line);
    if(head == NULL){
        head = temp;
    }else{
        temp->next = head;
        head = temp;
    }
    return head;
}
  
node DelHead(node head){
    if(head == NULL){
        printf("\nCha co gi de xoa het!");
    }else{
        head = head->next;
    }
    return head;
}
 
node DelTail(node head){
    if (head == NULL || head->next == NULL){
         return DelHead(head);
    }
    node p = head;
    while(p->next->next != NULL){
        p = p->next;
    }
    p->next = p->next->next; // Cho next bằng NULL
    // Hoặc viết p->next = NULL cũng được
    return head;
}
int Update(node head, char* value, int line){
    for(node p = head; p != NULL; p = p->next){
        if(strcmp(p->data, value) == 0){
            if(p->lines[p->idx] != line) {
                p->idx++;
                p->lines[p->idx] = line;
            }
            return 1;
        }
    }
    return 0;
}
 
node InitHead(){
    node head;
    head = NULL;
    return head;
}
 
void travel(node head){
    for(node p = head; p != NULL; p = p->next){
        printf("- %s\n", p->data);
        for (int i = 0; i <= p->idx; i++) {
            printf("%d\n", p->lines[i]);
        }
    }
}

int lenOfNode(node head) {
    int len = 0;
    for(node p = head; p != NULL; p = p->next){
        len++;
    }
    return len;
}
