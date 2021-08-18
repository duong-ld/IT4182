/*
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#ifndef __TOKEN_H__
#define __TOKEN_H__

#define MAX_IDENT_LEN 15
#define MAX_STRING_LEN 99
#define KEYWORDS_COUNT 25

typedef enum {
  TK_NONE,
  TK_IDENT,
  TK_NUMBER,
  TK_CHAR,
  TK_EOF,
  // final term
  // thêm double và string
  TK_DOUBLE,
  TK_STRING,
  // end

  KW_PROGRAM,
  KW_CONST,
  KW_TYPE,
  KW_VAR,
  KW_STRING,
  KW_DOUBLE,
  KW_INTEGER,
  KW_CHAR,
  KW_ARRAY,
  KW_OF,
  KW_FUNCTION,
  KW_PROCEDURE,
  KW_BEGIN,
  KW_END,
  KW_CALL,
  KW_IF,
  KW_THEN,
  KW_ELSE,
  KW_WHILE,
  KW_DO,
  KW_FOR,
  KW_TO,
  // final term
  // phép gán có if, else
  KW_RETURN,
  // câu lệnh repaeat <exp> until <condition>
  KW_REPEAT,
  KW_UNTIL,
  // end

  SB_SEMICOLON,
  SB_COLON,
  SB_PERIOD,
  SB_COMMA,
  SB_ASSIGN,
  SB_EQ,
  SB_NEQ,
  SB_LT,
  SB_LE,
  SB_GT,
  SB_GE,
  SB_PLUS,
  SB_MINUS,
  SB_TIMES,
  SB_SLASH,
  SB_LPAR,
  SB_RPAR,
  SB_LSEL,
  SB_RSEL,
  // final term
  // dấu hỏi chấm trong phép gán 3 ngôi
  // x = a > b ? c : d
  SB_QUESTION,
  // phép gán có if, else
  // ::=
  SB_ASSIGN_2
} TokenType;

typedef struct {
  char string[MAX_IDENT_LEN + 1];
  int lineNo, colNo;
  TokenType tokenType;
  double value;
} Token;

TokenType checkKeyword(char* string);
Token* makeToken(TokenType tokenType, int lineNo, int colNo);
char* tokenToString(TokenType tokenType);

#endif
