/*
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#include "semantics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

extern SymTab* symtab;
extern Token* currentToken;

Object* lookupObject(char* name) {
  Scope* scope = symtab->currentScope;
  Object* obj;

  while (scope != NULL) {
    obj = findObject(scope->objList, name);
    if (obj != NULL)
      return obj;
    scope = scope->outer;
  }
  obj = findObject(symtab->globalObjectList, name);
  if (obj != NULL)
    return obj;
  return NULL;
}

void checkFreshIdent(char* name) {
  if (findObject(symtab->currentScope->objList, name) != NULL)
    error(ERR_DUPLICATE_IDENT, currentToken->lineNo, currentToken->colNo);
}

Object* checkDeclaredIdent(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL) {
    error(ERR_UNDECLARED_IDENT, currentToken->lineNo, currentToken->colNo);
  }
  return obj;
}

Object* checkDeclaredConstant(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    error(ERR_UNDECLARED_CONSTANT, currentToken->lineNo, currentToken->colNo);
  if (obj->kind != OBJ_CONSTANT)
    error(ERR_INVALID_CONSTANT, currentToken->lineNo, currentToken->colNo);

  return obj;
}

Object* checkDeclaredType(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    error(ERR_UNDECLARED_TYPE, currentToken->lineNo, currentToken->colNo);
  if (obj->kind != OBJ_TYPE)
    error(ERR_INVALID_TYPE, currentToken->lineNo, currentToken->colNo);

  return obj;
}

Object* checkDeclaredVariable(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    error(ERR_UNDECLARED_VARIABLE, currentToken->lineNo, currentToken->colNo);
  if (obj->kind != OBJ_VARIABLE)
    error(ERR_INVALID_VARIABLE, currentToken->lineNo, currentToken->colNo);

  return obj;
}

Object* checkDeclaredFunction(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    error(ERR_UNDECLARED_FUNCTION, currentToken->lineNo, currentToken->colNo);
  if (obj->kind != OBJ_FUNCTION)
    error(ERR_INVALID_FUNCTION, currentToken->lineNo, currentToken->colNo);

  return obj;
}

Object* checkDeclaredProcedure(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    error(ERR_UNDECLARED_PROCEDURE, currentToken->lineNo, currentToken->colNo);
  if (obj->kind != OBJ_PROCEDURE)
    error(ERR_INVALID_PROCEDURE, currentToken->lineNo, currentToken->colNo);

  return obj;
}

Object* checkDeclaredLValueIdent(char* name) {
  Object* obj = lookupObject(name);
  if (obj == NULL)
    error(ERR_UNDECLARED_IDENT, currentToken->lineNo, currentToken->colNo);

  switch (obj->kind) {
  case OBJ_VARIABLE:
  case OBJ_PARAMETER:
    break;
  case OBJ_FUNCTION:
    if (obj != symtab->currentScope->owner) {
      error(ERR_INVALID_IDENT, currentToken->lineNo, currentToken->colNo);
    }
    break;
  default:
    error(ERR_INVALID_IDENT, currentToken->lineNo, currentToken->colNo);
  }

  return obj;
}

void checkIntType(Type* type) {
  if (type == NULL || type->typeClass != TP_INT)
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
}

void checkCharType(Type* type) {
  if (type == NULL || type->typeClass != TP_CHAR)
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
}

void checkDoubleType(Type* type) {
  if (type == NULL || type->typeClass != TP_DOUBLE)
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
}

void checkFloatType(Type* type) {
  if (type == NULL || type->typeClass != TP_FLOAT)
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
}

void checkDoubleAndLowerType(Type* type) {
  if (type->typeClass != TP_DOUBLE
      && type->typeClass != TP_INT
      && type->typeClass != TP_FLOAT
      && type->typeClass != TP_LONG
      && type->typeClass != TP_SHORT)
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
}

void checkFloatAndLowerType(Type* type) {
  if (type->typeClass != TP_INT
    && type->typeClass != TP_FLOAT
    && type->typeClass != TP_LONG
    && type->typeClass != TP_SHORT)
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
}

void checkLongAndLowerType(Type* type) {
  if (type->typeClass != TP_INT
    && type->typeClass != TP_LONG
    && type->typeClass != TP_SHORT)
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
}

void checkIntAndLowerType(Type* type) {
  if (type->typeClass != TP_INT && type->typeClass != TP_SHORT)
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
}

void checkNumberType(Type* type) {
  if (type == NULL)
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
  checkDoubleAndLowerType(type);
}

/**
 * @brief Check the typeCheck in accordance with the typeSrc
 *          if typeSrc is number, lower type is OK
 *          else typeCheck must equality typeSrc
 *
 * @param typeSrc
 * @param typeCheck
 */
void checkSuitableType(Type* typeSrc, Type* typeCheck) {
  if (typeSrc == NULL || typeCheck == NULL)
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
  switch (typeSrc->typeClass) {
  case TP_INT:
    checkIntAndLowerType(typeCheck);
    break;
  case TP_LONG:
    checkLongAndLowerType(typeCheck);
    break;
  case TP_FLOAT:
    checkFloatAndLowerType(typeCheck);
    break;
  case TP_DOUBLE:
    checkDoubleAndLowerType(typeCheck);
    break;
  default:
    checkTypeEquality(typeSrc, typeCheck);
  }
}

int isNumberType(Type* type) {
  return type != NULL &&
    (type->typeClass == TP_INT || type->typeClass == TP_FLOAT ||
      type->typeClass == TP_DOUBLE || type->typeClass == TP_LONG
      || type->typeClass == TP_SHORT);
}

void checkStringType(Type* type) {
  if (type == NULL || type->typeClass != TP_STRING)
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
}

void checkBasicType(Type* type) {
  if (type == NULL ||
    (type->typeClass != TP_INT
      && type->typeClass != TP_CHAR
      && type->typeClass != TP_STRING
      && type->typeClass != TP_DOUBLE
      && type->typeClass != TP_FLOAT
      && type->typeClass != TP_LONG
      && type->typeClass != TP_SHORT))
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
}

void checkArrayType(Type* type) {
  if (type == NULL || type->typeClass != TP_ARRAY)
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
}

/**
 * @brief input two type of number and output is the higher type
 *        order type: short->int->long->float->double
 *
 * @param type1
 * @param type2 
 * @return Type* 
 */
Type* autoUpcasting(Type* type1, Type* type2) {
  checkNumberType(type1);
  checkNumberType(type2);

  if (type1->typeClass == TP_DOUBLE) {
    return type1;
  }
  if (type2->typeClass == TP_DOUBLE) {
    return type2;
  }
  if (type1->typeClass == TP_FLOAT) {
    return type1;
  }
  if (type2->typeClass == TP_FLOAT) {
    return type2;
  }
  if (type1->typeClass == TP_LONG) {
    return type1;
  }
  if (type2->typeClass == TP_LONG) {
    return type2;
  }
  if (type1->typeClass == TP_INT) {
    return type1;
  }
  else {
    return type2;
  }
}

void checkTypeEquality(Type* type1, Type* type2) {
  if (type1 == NULL || type2 == NULL) {
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
  }

  if (type1->typeClass == type2->typeClass) {
    if (type1->typeClass == TP_ARRAY) {
      if (type1->arraySize == type2->arraySize)
        return checkTypeEquality(type1->elementType, type2->elementType);
      else
        error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo,
          currentToken->colNo);
    }
  }
  else
    error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
}
