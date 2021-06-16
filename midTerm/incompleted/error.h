/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#ifndef __ERROR_H__
#define __ERROR_H__
#include "token.h"

typedef enum {
  ERR_END_OF_COMMENT,
  ERR_IDENT_TOO_LONG,
  ERR_INVALID_CONSTANT_CHAR,
  ERR_INVALID_CONSTANT_STRING,
  ERR_INVALID_SYMBOL,
  ERR_INVALID_IDENT,
  ERR_INVALID_CONSTANT,
  ERR_INVALID_TYPE,
  ERR_INVALID_BASICTYPE,
  ERR_INVALID_VARIABLE,
  ERR_INVALID_FUNCTION,
  ERR_INVALID_PROCEDURE,
  ERR_INVALID_PARAMETER,
  ERR_INVALID_STATEMENT,
  ERR_INVALID_COMPARATOR,
  ERR_INVALID_EXPRESSION,
  ERR_INVALID_TERM,
  ERR_INVALID_FACTOR,
  ERR_INVALID_LVALUE,
  ERR_INVALID_ARGUMENTS,
  ERR_UNDECLARED_IDENT,
  ERR_UNDECLARED_CONSTANT,
  ERR_UNDECLARED_NUMBER_CONSTANT,
  ERR_UNDECLARED_TYPE,
  ERR_UNDECLARED_VARIABLE,
  ERR_UNDECLARED_FUNCTION,
  ERR_UNDECLARED_PROCEDURE,
  ERR_DUPLICATE_IDENT,
  ERR_TYPE_INCONSISTENCY,
  ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY,
  ERR_ASSIGN_LEFT_LESS,
  ERR_ASSIGN_LEFT_MORE
} ErrorCode;

void error(ErrorCode err, int lineNo, int colNo);
void missingToken(TokenType tokenType, int lineNo, int colNo);
void assert(char *msg);

#endif
