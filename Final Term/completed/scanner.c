/* Scanner
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "charcode.h"
#include "error.h"
#include "reader.h"
#include "scanner.h"
#include "token.h"

extern int lineNo;
extern int colNo;
extern int currentChar;

extern CharCode charCodes[];

/***************************************************************/

void skipBlank() {
  while ((currentChar != EOF) && (charCodes[currentChar] == CHAR_SPACE ||
    charCodes[currentChar] == CHAR_NEWLINE))
    readChar();
}

void skipComment() {
  int state = 0;
  while ((currentChar != EOF) && (state < 2)) {
    switch (charCodes[currentChar]) {
    case CHAR_TIMES:
      state = 1;
      break;
    case CHAR_RPAR:
      if (state == 1)
        state = 2;
      else
        state = 0;
      break;
    default:
      state = 0;
    }
    readChar();
  }
  if (state != 2)
    error(ERR_END_OF_COMMENT, lineNo, colNo);
}


//  5 states of comment like C:
//  - state 0: /
//  - state 1: //
//  - state 2: /*
//  - state 3: /* ... *
//  - state 4: end of comment
//
//  comment in line: //
//    state 0 -> state 1 -> state 4
//  comment in block: /* ... */
//    state 0 -> state 2 -> state 3 -> state 4
void skipCommentLikeC() {
  int state = 0;

  if (charCodes[currentChar] == CHAR_SLASH) {
    state = 1;

  }
  else if (charCodes[currentChar] == CHAR_TIMES) {
    state = 2;
  }

  while ((currentChar != EOF) && (state == 1)) {
    readChar();
    if (charCodes[currentChar] == CHAR_NEWLINE) {
      state = 4;
    }
  }

  while ((currentChar != EOF) && (state == 2 || state == 3)) {
    readChar();
    if (charCodes[currentChar] == CHAR_TIMES) {
      state = 3;
    }
    else if (charCodes[currentChar] == CHAR_SLASH && state == 3) {
      state = 4;
    }
    else {
      state = 2;
    }
  }

  if (state != 4) {
    error(ERR_END_OF_COMMENT, lineNo, colNo);
  }
  readChar();
}

Token* readIdentKeyword(void) {
  Token* token = makeToken(TK_NONE, lineNo, colNo);
  int count = 1;

  token->string[0] = toupper((char)currentChar);
  readChar();

  while ((currentChar != EOF) && ((charCodes[currentChar] == CHAR_LETTER) ||
    (charCodes[currentChar] == CHAR_DIGIT))) {
    if (count <= MAX_IDENT_LEN)
      token->string[count++] = toupper((char)currentChar);
    readChar();
  }

  if (count > MAX_IDENT_LEN) {
    error(ERR_IDENT_TOO_LONG, token->lineNo, token->colNo);
  }

  token->string[count] = '\0';
  token->tokenType = checkKeyword(token->string);

  if (token->tokenType == TK_NONE)
    token->tokenType = TK_IDENT;

  return token;
}

/**
 * @param initState
 *         initState = 0: had met digit before. ex: 123.23
 *         initState = 1: had met '.' before. ex: .123
 * @return Token*
 */
Token* readNumber(int initState) {
  Token* token = makeToken(TK_NUMBER, lineNo, colNo);
  int count = 0;
  int checkDot = initState;
  int isFloat = 0;
  int isLong = 0;
  int isShort = 0;

  if (initState) {
    token->string[count++] = '0';
    token->string[count++] = '.';
  }
  while ((currentChar != EOF) && (charCodes[currentChar] == CHAR_DIGIT ||
    charCodes[currentChar] == CHAR_PERIOD)) {
    if (charCodes[currentChar] == CHAR_PERIOD && checkDot)
      break;
    if (charCodes[currentChar] == CHAR_PERIOD)
      checkDot = 1;

    token->string[count++] = (char)currentChar;
    readChar();
  }

  // if have f(102) or F(70) in the end of number => isFloat = 1
  // if have s(115) or S(83) in the end of number => isShort = 1
  // if have l(108) or L(76) in the end of number => isLong = 1
  if (currentChar == 70 || currentChar == 102) {
    isFloat = 1;
    readChar();
  }
  else if (currentChar == 83 || currentChar == 115) {
    isShort = 1;
    readChar();
  }
  else if (currentChar == 76 || currentChar == 108) {
    isLong = 1;
    readChar();
  }

  if (token->string[count - 1] == '.') {
    token->string[count] = '0';
    count++;
  }

  token->string[count] = '\0';

  if (!checkDot) {
    double value = atof(token->string);
    if (isShort) {
      if (value < SHRT_MIN || value > SHRT_MAX)
        error(ERR_OUT_OF_RANGE_TYPE, lineNo, colNo);
      token->tokenType = TK_SHORT;
      token->value = (short) value;
    }
    else if (isLong) {
      if (value < LONG_MIN || value > LONG_MAX)
        error(ERR_OUT_OF_RANGE_TYPE, lineNo, colNo);
      token->tokenType = TK_LONG;
      token->value = (long) value;
    }
    else {
      if (value < INT_MIN || value > INT_MAX)
        error(ERR_OUT_OF_RANGE_TYPE, lineNo, colNo);
      token->value = (int) value;
    }
    if (isFloat) error(ERR_INVALID_SYMBOL, lineNo, colNo);
  }
  else {
    token->value = atof(token->string);
    if (isFloat) token->tokenType = TK_FLOAT;
    else {
      token->tokenType = TK_DOUBLE;
    }
    if (isLong || isShort) error(ERR_INVALID_SYMBOL, lineNo, colNo);
  }

  return token;
}

Token* readConstChar(void) {
  Token* token = makeToken(TK_CHAR, lineNo, colNo);

  readChar();
  if (currentChar == EOF) {
    token->tokenType = TK_NONE;
    error(ERR_INVALID_CONSTANT_CHAR, token->lineNo, token->colNo);
  }

  token->string[0] = currentChar;
  token->string[1] = '\0';

  readChar();
  if (currentChar == EOF) {
    token->tokenType = TK_NONE;
    error(ERR_INVALID_CONSTANT_CHAR, token->lineNo, token->colNo);
  }

  if (charCodes[currentChar] == CHAR_SINGLEQUOTE) {
    readChar();
    return token;
  }
  else {
    token->tokenType = TK_NONE;
    error(ERR_INVALID_CONSTANT_CHAR, token->lineNo, token->colNo);
    DISABLE_RETURN_WARNING;
  }
}

Token* readConstString(void) {
  char* ident = (char*)calloc(MAX_STRING_LEN + 1, sizeof(char));

  int i = 0;
  readChar();
  Token* result = makeToken(TK_STRING, lineNo, colNo);

  do {
    if (currentChar == EOF) {
      error(ERR_INVALID_CONSTANT_STRING, lineNo, colNo);
    }
    if (charCodes[currentChar] == CHAR_DOUBLEQUOTE)
      break;
    if (i == MAX_STRING_LEN) {
      error(ERR_INVALID_CONSTANT_STRING, lineNo, colNo);
    }

    ident[i] = (char)currentChar;
    i++;
    readChar();
  } while (1);

  ident[i] = '\0';
  strcpy(result->string, ident);
  if (ident != NULL)
    free(ident);
  readChar();
  return result;
}

Token* getToken(void) {
  Token* token;
  int ln, cn;

  if (currentChar == EOF)
    return makeToken(TK_EOF, lineNo, colNo);

  switch (charCodes[currentChar]) {
  case CHAR_SPACE:
  case CHAR_NEWLINE:
    skipBlank();
    return getToken();
  case CHAR_LETTER:
    return readIdentKeyword();
  case CHAR_DIGIT:
    return readNumber(0);
  case CHAR_PLUS:
    token = makeToken(SB_PLUS, lineNo, colNo);
    readChar();
    return token;
  case CHAR_MINUS:
    token = makeToken(SB_MINUS, lineNo, colNo);
    readChar();
    return token;
  case CHAR_TIMES:
    ln = lineNo;
    cn = colNo;
    readChar();
    if ((currentChar != EOF) && (charCodes[currentChar] == CHAR_TIMES)) {
      readChar();
      return makeToken(SB_POWER, ln, cn);
    }
    else
      return makeToken(SB_TIMES, ln, cn);
    return token;
  case CHAR_SLASH:
    ln = lineNo;
    cn = colNo;
    readChar();
    if ((currentChar != EOF) && (charCodes[currentChar] == CHAR_SLASH ||
      charCodes[currentChar] == CHAR_TIMES)) {
      skipCommentLikeC();
      return getToken();
    }
    else {
      return makeToken(SB_SLASH, ln, cn);
    }
  case CHAR_POWER:
    token = makeToken(SB_POWER, lineNo, colNo);
    readChar();
    return token;
  case CHAR_LT:
    ln = lineNo;
    cn = colNo;
    readChar();
    if ((currentChar != EOF) && (charCodes[currentChar] == CHAR_EQ)) {
      readChar();
      return makeToken(SB_LE, ln, cn);
    }
    else
      return makeToken(SB_LT, ln, cn);
  case CHAR_GT:
    ln = lineNo;
    cn = colNo;
    readChar();
    if ((currentChar != EOF) && (charCodes[currentChar] == CHAR_EQ)) {
      readChar();
      return makeToken(SB_GE, ln, cn);
    }
    else
      return makeToken(SB_GT, ln, cn);
  case CHAR_EQ:
    token = makeToken(SB_EQ, lineNo, colNo);
    readChar();
    return token;
  case CHAR_EXCLAIMATION:
    ln = lineNo;
    cn = colNo;
    readChar();
    if ((currentChar != EOF) && (charCodes[currentChar] == CHAR_EQ)) {
      readChar();
      return makeToken(SB_NEQ, ln, cn);
    }
    else {
      token = makeToken(TK_NONE, ln, cn);
      error(ERR_INVALID_SYMBOL, ln, cn);
    }
  case CHAR_MOD:
    token = makeToken(SB_MOD, lineNo, colNo);
    readChar();
    return token;
  case CHAR_COMMA:
    token = makeToken(SB_COMMA, lineNo, colNo);
    readChar();
    return token;
  case CHAR_PERIOD:
    ln = lineNo;
    cn = colNo;
    readChar();
    if ((currentChar != EOF) && (charCodes[currentChar] == CHAR_RPAR)) {
      readChar();
      return makeToken(SB_RSEL, ln, cn);
    }
    else if ((currentChar != EOF) &&
      (charCodes[currentChar] == CHAR_DIGIT)) {
      return readNumber(1);
    }
    else
      return makeToken(SB_PERIOD, ln, cn);
  case CHAR_SEMICOLON:
    token = makeToken(SB_SEMICOLON, lineNo, colNo);
    readChar();
    return token;
  case CHAR_COLON:
    ln = lineNo;
    cn = colNo;
    readChar();
    if ((currentChar != EOF) && (charCodes[currentChar] == CHAR_EQ)) {
      readChar();
      return makeToken(SB_ASSIGN, ln, cn);
    }
    else if ((currentChar != EOF) && (charCodes[currentChar] == CHAR_COLON)) {
      readChar();
      if (charCodes[currentChar] == CHAR_EQ) {
        readChar();
        return makeToken(SB_ASSIGN_2, ln, cn);
      }
      else {
        error(ERR_INVALID_SYMBOL, ln, cn);
      }
    }
    else
      return makeToken(SB_COLON, ln, cn);
  case CHAR_SINGLEQUOTE:
    return readConstChar();
  case CHAR_DOUBLEQUOTE:
    return readConstString();
  case CHAR_LPAR:
    ln = lineNo;
    cn = colNo;
    readChar();

    if (currentChar == EOF)
      return makeToken(SB_LPAR, ln, cn);

    switch (charCodes[currentChar]) {
    case CHAR_PERIOD:
      readChar();
      return makeToken(SB_LSEL, ln, cn);
    case CHAR_TIMES:
      readChar();
      skipComment();
      return getToken();
    default:
      return makeToken(SB_LPAR, ln, cn);
    }
  case CHAR_RPAR:
    token = makeToken(SB_RPAR, lineNo, colNo);
    readChar();
    return token;
  case CHAR_RSEL:
    token = makeToken(SB_RSEL, lineNo, colNo);
    readChar();
    return token;
  case CHAR_LSEL:
    token = makeToken(SB_LSEL, lineNo, colNo);
    readChar();
    return token;
  case CHAR_QUESTION:
    token = makeToken(SB_QUESTION, lineNo, colNo);
    readChar();
    return token;
  default:
    token = makeToken(TK_NONE, lineNo, colNo);
    error(ERR_INVALID_SYMBOL, lineNo, colNo);
    DISABLE_RETURN_WARNING;
  }
}

Token* getValidToken(void) {
  printf("");
  Token* token = getToken();
  while (token->tokenType == TK_NONE) {
    free(token);
    token = getToken();
  }
  return token;
}

/******************************************************************/

void printToken(Token* token) {
  printf("%d-%d:", token->lineNo, token->colNo);

  switch (token->tokenType) {
  case TK_NONE:
    printf("TK_NONE\n");
    break;
  case TK_IDENT:
    printf("TK_IDENT(%s)\n", token->string);
    break;
  case TK_NUMBER:
    printf("TK_NUMBER(%s)\n", token->string);
    break;
  case TK_CHAR:
    printf("TK_CHAR(\'%s\')\n", token->string);
    break;
  case TK_EOF:
    printf("TK_EOF\n");
    break;
  case TK_DOUBLE:
    printf("TK_DOUBLE\n");
    break;
  case TK_FLOAT:
    printf("TK_FLOAT\n");
    break;
  case TK_SHORT:
    printf("TK_SHORT\n");
    break;
  case TK_LONG:
    printf("TK_LONG\n");
    break;
  case TK_STRING:
    printf("TK_STRING\n");
    break;

  case KW_PROGRAM:
    printf("KW_PROGRAM\n");
    break;
  case KW_CONST:
    printf("KW_CONST\n");
    break;
  case KW_TYPE:
    printf("KW_TYPE\n");
    break;
  case KW_VAR:
    printf("KW_VAR\n");
    break;
  case KW_INTEGER:
    printf("KW_INTEGER\n");
    break;
  case KW_CHAR:
    printf("KW_CHAR\n");
    break;
  case KW_ARRAY:
    printf("KW_ARRAY\n");
    break;
  case KW_OF:
    printf("KW_OF\n");
    break;
  case KW_FUNCTION:
    printf("KW_FUNCTION\n");
    break;
  case KW_PROCEDURE:
    printf("KW_PROCEDURE\n");
    break;
  case KW_BEGIN:
    printf("KW_BEGIN\n");
    break;
  case KW_END:
    printf("KW_END\n");
    break;
  case KW_CALL:
    printf("KW_CALL\n");
    break;
  case KW_IF:
    printf("KW_IF\n");
    break;
  case KW_THEN:
    printf("KW_THEN\n");
    break;
  case KW_ELSE:
    printf("KW_ELSE\n");
    break;
  case KW_WHILE:
    printf("KW_WHILE\n");
    break;
  case KW_DO:
    printf("KW_DO\n");
    break;
  case KW_FOR:
    printf("KW_FOR\n");
    break;
  case KW_TO:
    printf("KW_TO\n");
    break;
  case KW_STRING:
    printf("KW_STRING\n");
    break;
  case KW_DOUBLE:
    printf("KW_DOUBLE\n");
    break;
  case KW_FLOAT:
    printf("KW_FLOAT\n");
    break;
  case KW_SHORT:
    printf("KW_SHORT\n");
    break;
  case KW_LONG:
    printf("KW_LONG\n");
    break;
  case KW_RETURN:
    printf("KW_RETURN\n");
  case KW_REPEAT:
    printf("KW_REPEAT\n");
  case KW_UNTIL:
    printf("KW_UNTIL\n");
  case KW_SWITCH:
    printf("KW_SWITCH\n");
  case KW_CASE:
    printf("KW_CASE\n");
  case KW_DEFAULT:
    printf("KW_DEFAULT\n");
  case KW_BREAK:
    printf("KW_BREAK\n");
  case KW_SUM:
    printf("KW_SUM\n");

  case SB_SEMICOLON:
    printf("SB_SEMICOLON\n");
    break;
  case SB_COLON:
    printf("SB_COLON\n");
    break;
  case SB_PERIOD:
    printf("SB_PERIOD\n");
    break;
  case SB_MOD:
    printf("SB_MOD\n");
    break;
  case SB_COMMA:
    printf("SB_COMMA\n");
    break;
  case SB_ASSIGN:
    printf("SB_ASSIGN\n");
    break;
  case SB_ASSIGN_2:
    printf("SB_ASSIGN_2\n");
    break;
  case SB_EQ:
    printf("SB_EQ\n");
    break;
  case SB_NEQ:
    printf("SB_NEQ\n");
    break;
  case SB_LT:
    printf("SB_LT\n");
    break;
  case SB_LE:
    printf("SB_LE\n");
    break;
  case SB_GT:
    printf("SB_GT\n");
    break;
  case SB_GE:
    printf("SB_GE\n");
    break;
  case SB_PLUS:
    printf("SB_PLUS\n");
    break;
  case SB_MINUS:
    printf("SB_MINUS\n");
    break;
  case SB_TIMES:
    printf("SB_TIMES\n");
    break;
  case SB_SLASH:
    printf("SB_SLASH\n");
    break;
  case SB_LPAR:
    printf("SB_LPAR\n");
    break;
  case SB_RPAR:
    printf("SB_RPAR\n");
    break;
  case SB_LSEL:
    printf("SB_LSEL\n");
    break;
  case SB_RSEL:
    printf("SB_RSEL\n");
    break;
  case SB_QUESTION:
    printf("SB_QUESTION\n");
    break;
  case SB_POWER:
    printf("SB_POWER\n");
    break;
  }
}
