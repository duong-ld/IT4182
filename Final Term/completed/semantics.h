/*
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#ifndef __SEMANTICS_H__
#define __SEMANTICS_H__

#include "symtab.h"

void checkFreshIdent(char* name);
Object* checkDeclaredIdent(char* name);
Object* checkDeclaredConstant(char* name);
Object* checkDeclaredType(char* name);
Object* checkDeclaredVariable(char* name);
Object* checkDeclaredFunction(char* name);
Object* checkDeclaredProcedure(char* name);
Object* checkDeclaredLValueIdent(char* name);

void checkIntType(Type* type);
void checkCharType(Type* type);
void checkDoubleType(Type* type);
void checkFloatType(Type* type);
void checkNumberType(Type* type);
void checkDoubleAndLowerType(Type* type);
void checkFloatAndLowerType(Type* type);
void checkLongAndLowerType(Type* type);
void checkIntAndLowerType(Type* type);
void checkSuitableType(Type* typeSrc, Type* typeCheck);
int isNumberType(Type* type);
void checkStringType(Type* type);
void checkArrayType(Type* type);
void checkBasicType(Type* type);
void checkTypeEquality(Type* type1, Type* type2);
Type* autoUpcasting(Type* type1, Type* type2);

#endif
