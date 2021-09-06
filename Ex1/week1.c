//
//  week1.c
//  indexWord
//
//  Created by Lương Dương on 24/02/2021
//
//  Link to github:
//  https://github.com/DuongLD-140800/IT4182.git
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "const.h"
#include "singleLList.h"

// global var //
char **G_strStop = NULL;
int G_linesStop = 0;
int G_checkDot = 1;
char G_endOfWord[] = " ,.;:?(){}[]-<>!@#$%^&*'";
///////////////////////////////

int countLine(FILE *fp) {
    int lines = 0;
    char ch;
    while(!feof(fp)) {
        ch = fgetc(fp);
        if(ch == '\n') {
            lines++;
        }
    }
    rewind(fp);
    return lines;
}
void free2lvlPointer(char **str, int n) {
    for (int i = 0; i < n; i++) {
        if (str[i] != NULL) free(str[i]);
    }
    free(str);
}
// read file then return array of String.
// each string is one line in text
char **readFile(FILE *fp, int lines) {
    char **strLine = NULL;
    int lenOfWord = 0;
    strLine = (char **) malloc(lines * sizeof(char*));
    if (strLine == NULL) {
        printf("Cannot malloc! Read file fail\n");
        return NULL;
    }
    for (int i = 0; i < lines; i++) {
        strLine[i] = (char *) malloc(C_MAX_LINE * sizeof(char));
        fscanf(fp, "%[^\n]s", strLine[i]);
        lenOfWord = (int) strlen(strLine[i]);
        
        // check CR (13 in ASCII code)
        if ((int) strLine[i][lenOfWord - 1] == 13)
            strLine[i][lenOfWord - 1] = '\0';
        fgetc(fp);
    }
    return strLine;
}

int checkEndWord(char c) {
    if (c == '\t') return 1;
    for (int i = 0; i < strlen(G_endOfWord); i++) {
        if (c == G_endOfWord[i]) return 1;
    }
    return 0;
}

int checkInStop(char *str) {
    for (int i = 0; i < G_linesStop; i++) {
        if (strcmp(str, G_strStop[i]) == 0)
            return 1;
    }
    return 0;
}

void toLowerCase(char *str) {
    for(int i = 0; i <= strlen(str); i++){
          if(str[i] >= 65 && str[i] <= 90)
             str[i] = str[i]+32;
    }
}

node handleStr(char *str, int line, node head) {
    char word[C_MAX_LENW] = "";
    int idx_word = 0;
    for (int i = 0; i < strlen(str); i++) {
        if (checkEndWord(str[i])) {
            if (str[i] == '.' || str[i] == '\t') G_checkDot = 1;
            if (strcmp(word, "") != 0 &&
                'a' <= word[0] && word[0] <= 'z' &&
                !checkInStop(word))
            {
                toLowerCase(word);
                // if update fail then this word not in link-list
                if (!Update(head, word, line))
                    head = AddHead(head, word, line);
            }
            // reset word
            memset(word, 0, strlen(word));
            idx_word = 0;
            continue;
        }
        if ('A' <= str[i] && str[i] <= 'Z' && G_checkDot == 1) {
            word[idx_word] = str[i] + 32;
            G_checkDot = 0;
            idx_word++;
        } else {
            word[idx_word] = str[i];
            G_checkDot = 0;
            idx_word++;
        }
    }
    // the last word
    if (strcmp(word, "") != 0 &&
        'a' <= word[0] && word[0] <= 'z' &&
        !checkInStop(word))
    {
        toLowerCase(word);
        if (!Update(head, word, line))
            head = AddHead(head, word, line);
    }
    return head;
}
node handleTxt(char **strTxt, int linesTxt, node head) {
    for (int i = 0; i < linesTxt; i++) {
        head = handleStr(strTxt[i], i + 1, head);
    }
    return head;
}
int myCompare( const void* a, const void* b ) {
  return strcmp( *(const char**)a, *(const char**)b );
}
void printResult(node head) {
    char **str = (char**) malloc(sizeof(char *) * lenOfNode(head));
    if (str == NULL) {
        printf("Cannot malloc! fail in print result\n");
        return;
    }
    int i = 0;
    // format print
    // <node->data> <node->lines[1]>,<node->lines[2]>,...
    for (node p = head; p != NULL; p = p->next) {
        str[i] = (char*) malloc(sizeof(char) * C_MAX_LENO);
        strcpy(str[i], p->data);
        str[i][strlen(str[i])] = ' ';
        for (int j = 0; j <= p->idx; j++) {
            char tmp[10] = "";
            sprintf(tmp, "%d", p->lines[j]);
            strcat(str[i], tmp);
            if (j != p->idx) strcat(str[i], ",");
        }
        i++;
    }
    // sort by alphabet
    qsort(str, lenOfNode(head), sizeof(char*), myCompare);
    // output: file <result.txt> && terminal
    FILE *fResult = fopen("result.txt", "w");
    if (fResult == NULL) {
        printf("Cannot create file <result.txt>\n");
        return;
    }
    for (int j = 0; j < i; j++) {
        fprintf(fResult, "%s\n", str[j]);
    }
    // clean
    free2lvlPointer(str, i);
    fclose(fResult);
}
int main(int argc, const char * argv[]) {
    if (argc < 3) {
        printf("File name is null!\n");
        printf("Try: ./ex1 [file_txt] [file_stop]\n");
        return 1;
    }
    FILE *ftxt = fopen(argv[1], "r");
    FILE *fstop = fopen(argv[2], "r");
    if (ftxt == NULL || fstop == NULL) {
        printf("Cannot open file!");
        return  2;
    }
    // count lines in txt
    int linesTxt = countLine(ftxt);
    G_linesStop = countLine(fstop);
    // Read file
    char **strTxt = readFile(ftxt, linesTxt);
    G_strStop = readFile(fstop, G_linesStop);
    // create linkList
    node head = InitHead();
    head = handleTxt(strTxt, linesTxt, head);
    printResult(head);
    // clean
    fclose(ftxt);
    fclose(fstop);
    free2lvlPointer(strTxt, linesTxt);
    free2lvlPointer(G_strStop, G_linesStop);
    return 0;
}

