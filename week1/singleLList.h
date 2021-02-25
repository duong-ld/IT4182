//
//  singleLList.h
//  indexWord
//
//  Created by Lương Dương on 24/02/2021.
//

#ifndef linkList_h
#define linkList_h

struct LinkedList{
    char data[C_MAX_LENW];
    int lines[C_MAX_FREQ];
    int idx;
    int times;
    struct LinkedList *next;
 };
 
typedef struct LinkedList *node;

node CreateNode(char* value, int line);
node AddTail(node head, char* value, int line);
node AddHead(node head, char* value, int line);
node DelHead(node head);
node DelTail(node head);
int Update(node head, char* value, int line);
node InitHead(void);
void travel(node head);
int lenOfNode(node head);

#endif /* singleLList_h */
