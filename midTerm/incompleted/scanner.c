/* Scanner
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// final term
// thêm cách comment của C vào KPL
// coment của C:
// - // comment trên 1 dòng
// - /* comment trên n dòng */
// chia làm 5 state
void skipCommentLikeC() {
  // state 0: đã nhận được / đầu tiên, chờ * hoặc / để tiến lên các state khác
  int state = 0;
  // state 1: nhận được / tức đây là //: comment trên cùng 1 dòng
  if (charCodes[currentChar] == CHAR_SLASH) {
    state = 1;

  }
  // state 2: nhận được /*: comment trên n dòng
  else if (charCodes[currentChar] == CHAR_TIMES) {
    state = 2;
  }

  // comment trên cùng một dòng sẽ kết thúc khi gặp kí tự xuống dòng
  while ((currentChar != EOF) && (state == 1)) {
    readChar();
    // Nếu gặp kí tự xuống dòng chuyển sang state 4
    if (charCodes[currentChar] == CHAR_NEWLINE) {
      state = 4;
    }
  }
  // comment trên n dòng sẽ kết thúc khi gặp */
  while ((currentChar != EOF) && (state == 2 || state == 3)) {
    readChar();
    if (charCodes[currentChar] == CHAR_TIMES) {
      // state 3: đã gặp * và đang chờ / để tạo thành */
      state = 3;
    }
    // Nếu gặp / mà trước đó có * đã được */
    // chuyển sang state 4
    else if (charCodes[currentChar] == CHAR_SLASH && state == 3) {
      state = 4;
      // Nếu gặp các kí tự khác tức là vẫn đang trong đoạn comment chuyển state
      // 2
    } else {
      state = 2;
    }
  }

  // Nếu kết thúc mà state khác 4 tức là chưa kết thúc comment
  // comment bị lỗi
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

// final term
// đọc số trong đó có cả số thập phân
// số thập phân có 2 trường hợp:
// TH1: .123 => 0.123
// TH2: 1.23 => 1.23
// TH3: 123. => 123.0
// Trường hợp 2, 3 khá là dễ, chỉ cần đọc được số thì sẽ nhảy vào hàm này
// ví dụ: đọc được 1 thì sẽ nhảy vào hàm readNumber và đọc tiếp
// Trường hợp đầu tiên khá khó vì đọc được . thì phải đọc tiếp
// Nếu sau . là số thì sẽ nhảy vào hàm readNumber
// Vì số thập phân ko thể là 0.123.123: có 2 dấu . trở lên được
// cần 1 biến checkDot: để check xem đã nhận dấu chấm nào chưa
// Và initState để chỉ trang thái dấu chấm đầu vào của hàm
// Vì khi đọc chuỗi sẽ đọc kí tự đầu tiên rồi nhảy vào hàm
// Nếu kí tự đầu tiên được đọc đó là dấu chấm tức là initState = 1 còn lại là 0
// checkDot = initState
// Ví dụ: .123 thì initState = 1 => checkDot = 1
//        Đã đọc 1 dấu chấm rồi, không cho phép xuất hiện dấu chấm nữa
//        1.23 thì initState = 0 => checkDot = 0
//        Chưa xuất hiện dấu chấm
Token* readNumber(int initState) {
  Token* token = makeToken(TK_NUMBER, lineNo, colNo);
  int count = 0;
  int checkDot = initState;

  // Nếu initState = 1 tức là gặp dấu . ở ngay đầu
  // Ví dụ: .123, .345
  // => tự động thêm 0. vào đầu chuỗi
  // sẽ thành 0.123, 0.345
  if (initState) {
    token->string[count++] = '0';
    token->string[count++] = '.';
  }
  while ((currentChar != EOF) && (charCodes[currentChar] == CHAR_DIGIT ||
                                  charCodes[currentChar] == CHAR_PERIOD)) {
    if (charCodes[currentChar] == CHAR_PERIOD && checkDot == 1)
      break;
    if (charCodes[currentChar] == CHAR_PERIOD)
      checkDot = 1;

    token->string[count++] = (char)currentChar;
    readChar();
  }

  if (token->string[count - 1] == '.') {
    token->string[count] = '0';
    count++;
  }

  token->string[count] = '\0';

  if (!checkDot)
    token->value = atoi(token->string);
  else {
    token->value = atof(token->string);
    token->tokenType = TK_DOUBLE;
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
  } else {
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
    // final term
    // Đoán nhận SB_POWER: **
    // Nhận được 1 *
    // đọc tiếp kí tự tiếp theo
    // Nếu kí tự tiếp theo là * thì là SB_POWER
    // Nếu không thì trả về SB_TIMES (phép nhân)
    case CHAR_TIMES:
      ln = lineNo;
      cn = colNo;
      readChar();
      if ((currentChar != EOF) && (charCodes[currentChar] == CHAR_TIMES)) {
        readChar();
        return makeToken(SB_POWER, ln, cn);
      } else
        return makeToken(SB_TIMES, ln, cn);
      return token;
    case CHAR_SLASH:
      ln = lineNo;
      cn = colNo;
      readChar();
      // final term
      // gặp /
      // Nếu char tiếp theo là / hoặc * tức đây là comment trong c
      if ((currentChar != EOF) && (charCodes[currentChar] == CHAR_SLASH ||
                                   charCodes[currentChar] == CHAR_TIMES)) {
        skipCommentLikeC();
        return getToken();
      }
      // Nếu không trả về SB_SLASH
      else
        return makeToken(SB_SLASH, ln, cn);
    case CHAR_LT:
      ln = lineNo;
      cn = colNo;
      readChar();
      if ((currentChar != EOF) && (charCodes[currentChar] == CHAR_EQ)) {
        readChar();
        return makeToken(SB_LE, ln, cn);
      } else
        return makeToken(SB_LT, ln, cn);
    case CHAR_GT:
      ln = lineNo;
      cn = colNo;
      readChar();
      if ((currentChar != EOF) && (charCodes[currentChar] == CHAR_EQ)) {
        readChar();
        return makeToken(SB_GE, ln, cn);
      } else
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
      } else {
        token = makeToken(TK_NONE, ln, cn);
        error(ERR_INVALID_SYMBOL, ln, cn);
      }
    // final term
    // dấu % trong phép chia module
    case CHAR_MOD:
      token = makeToken(SB_MOD, lineNo, colNo);
      readChar();
      return token;
    case CHAR_COMMA:
      token = makeToken(SB_COMMA, lineNo, colNo);
      readChar();
      return token;
    // final term
    // đọc được dấu chấm
    // có 3 trường hợp:
    //      là dấu chấm
    //      là 1 phần của ngoặc mảng: .)
    //      là bắt đầu của 1 số thập phân .123
    case CHAR_PERIOD:
      ln = lineNo;
      cn = colNo;
      readChar();
      if ((currentChar != EOF) && (charCodes[currentChar] == CHAR_RPAR)) {
        readChar();
        return makeToken(SB_RSEL, ln, cn);
      } else if ((currentChar != EOF) &&
                 (charCodes[currentChar] == CHAR_DIGIT)) {
        return readNumber(1);
      } else
        return makeToken(SB_PERIOD, ln, cn);
    case CHAR_SEMICOLON:
      token = makeToken(SB_SEMICOLON, lineNo, colNo);
      readChar();
      return token;
    case CHAR_COLON:
      ln = lineNo;
      cn = colNo;
      readChar();
      // final term
      // gặp :
      // Nếu gặp = tức là phép gán thường :=
      // trả về token :=
      if ((currentChar != EOF) && (charCodes[currentChar] == CHAR_EQ)) {
        readChar();
        return makeToken(SB_ASSIGN, ln, cn);
      }
      // Nếu gặp : tức là phép gán if, else
      // trả về token ::=
      else if ((currentChar != EOF) && (charCodes[currentChar] == CHAR_COLON)) {
        readChar();
        readChar();
        return makeToken(SB_ASSIGN_2, ln, cn);
        // Trường hợp còn lại chỉ trả về SB_COLON
      } else
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
