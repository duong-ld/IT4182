/*
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "error.h"
#include "parser.h"
#include "reader.h"
#include "scanner.h"
#include "semantics.h"

Token* currentToken;
Token* lookAhead;

extern Type* intType;
extern Type* charType;
extern Type* doubleType;
extern Type* stringType;
extern SymTab* symtab;

void scan(void) {
  Token* tmp = currentToken;
  currentToken = lookAhead;
  lookAhead = getValidToken();
  free(tmp);
}

void eat(TokenType tokenType) {
  if (lookAhead->tokenType == tokenType) {
    scan();
  }
  else
    missingToken(tokenType, lookAhead->lineNo, lookAhead->colNo);
}

void compileProgram(void) {
  Object* program;

  eat(KW_PROGRAM);
  eat(TK_IDENT);

  program = createProgramObject(currentToken->string);
  enterBlock(program->progAttrs->scope);

  eat(SB_SEMICOLON);

  compileBlock();
  eat(SB_PERIOD);

  exitBlock();
}

void compileBlock(void) {
  Object* constObj;
  ConstantValue* constValue;

  if (lookAhead->tokenType == KW_CONST) {
    eat(KW_CONST);

    do {
      eat(TK_IDENT);

      checkFreshIdent(currentToken->string);
      constObj = createConstantObject(currentToken->string);

      eat(SB_EQ);
      constValue = compileConstant();

      constObj->constAttrs->value = constValue;
      declareObject(constObj);

      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock2();
  }
  else
    compileBlock2();
}

void compileBlock2(void) {
  Object* typeObj;
  Type* actualType;

  if (lookAhead->tokenType == KW_TYPE) {
    eat(KW_TYPE);

    do {
      eat(TK_IDENT);

      checkFreshIdent(currentToken->string);
      typeObj = createTypeObject(currentToken->string);

      eat(SB_EQ);
      actualType = compileType();

      typeObj->typeAttrs->actualType = actualType;
      declareObject(typeObj);

      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock3();
  }
  else
    compileBlock3();
}

// final term
// var x, y, z : Integer;
// cho phép gán nhiều var một lúc
void compileBlock3(void) {
  Object** varObjList = (Object**) malloc(sizeof(Object*) * MAX_ASSIGN);
  int index = 0;
  Type* varType;
  
  // keyword var ở ngay đầu đoạn khai báo biến
  if (lookAhead->tokenType == KW_VAR) {
    eat(KW_VAR);

    // khai báo các biến
    do {
      // mỗi dòng có thể có nhiều biến thay vì chỉ là 1 biến như ban đầu
      // ban đầu var x: integer;
      // lúc sau var x, y, z: integer;
      // cần một mảng varObjList để chứa các biến thay vì chỉ varObj
      // index là chỉ số của mảng
      // bắt đầu một dòng reset index về 0
      index = 0;

    READVAR:
      // đọc lần lượt các định danh
      // kiểm tra xem định danh đã tồn tại chưa
      // nếu tồn tại thì báo lỗi
      // nếu chưa tồn tại thì tạo mới thêm vào mảng varObjList
      // tăng chỉ số của mảng
      // kiểm tra xem sau biến vừa khai báo có phải là biến mới không
      // Nếu có, tiếp tục vòng lặp
      eat(TK_IDENT);
      checkFreshIdent(currentToken->string);
      varObjList[index++] = createVariableObject(currentToken->string);
      if (lookAhead->tokenType == SB_COMMA) {
        eat(SB_COMMA);
        if (lookAhead->tokenType == TK_IDENT) {
          goto READVAR;
        }
        else {
          missingToken(TK_IDENT, lookAhead->lineNo, lookAhead->colNo);
        }
      }

      eat(SB_COLON);
      varType = compileType();

      for (int i = 0; i < index; i++) {
        varObjList[i]->varAttrs->type = varType;
        declareObject(varObjList[i]);
      }

      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock4();
  }
  else
    compileBlock4();

  free(varObjList);
}

void compileBlock4(void) {
  compileSubDecls();
  compileBlock5();
}

void compileBlock5(void) {
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileSubDecls(void) {
  while ((lookAhead->tokenType == KW_FUNCTION) ||
    (lookAhead->tokenType == KW_PROCEDURE)) {
    if (lookAhead->tokenType == KW_FUNCTION)
      compileFuncDecl();
    else
      compileProcDecl();
  }
}

void compileFuncDecl(void) {
  Object* funcObj;
  Type* returnType;

  eat(KW_FUNCTION);
  eat(TK_IDENT);

  checkFreshIdent(currentToken->string);
  funcObj = createFunctionObject(currentToken->string);
  declareObject(funcObj);

  enterBlock(funcObj->funcAttrs->scope);

  compileParams();

  eat(SB_COLON);
  returnType = compileBasicType();
  funcObj->funcAttrs->returnType = returnType;

  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_SEMICOLON);

  exitBlock();
}

void compileProcDecl(void) {
  Object* procObj;

  eat(KW_PROCEDURE);
  eat(TK_IDENT);

  checkFreshIdent(currentToken->string);
  procObj = createProcedureObject(currentToken->string);
  declareObject(procObj);

  enterBlock(procObj->procAttrs->scope);

  compileParams();

  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_SEMICOLON);

  exitBlock();
}

ConstantValue* compileUnsignedConstant(void) {
  ConstantValue* constValue = NULL;
  Object* obj;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_DOUBLE:
    eat(TK_DOUBLE);
    constValue = makeDoubleConstant(currentToken->value);
    break;
  case TK_IDENT:
    eat(TK_IDENT);

    obj = checkDeclaredConstant(currentToken->string);
    constValue = duplicateConstantValue(obj->constAttrs->value);

    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  case TK_STRING:
    eat(TK_STRING);
    constValue = makeStringConstant(currentToken->string);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

ConstantValue* compileConstant(void) {
  ConstantValue* constValue = NULL;

  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    constValue = compileConstant2();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    constValue = compileConstant2();
    if (constValue->type == TP_INT) {
      constValue->intValue = -constValue->intValue;
    }
    else if (constValue->type == TP_DOUBLE) {
      constValue->doubleValue = -constValue->doubleValue;
    }
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  case TK_STRING:
    eat(TK_STRING);
    constValue = makeStringConstant(currentToken->string);
    break;
  default:
    constValue = compileConstant2();
    break;
  }
  return constValue;
}

ConstantValue* compileConstant2(void) {
  ConstantValue* constValue = NULL;
  Object* obj;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_DOUBLE:
    eat(TK_DOUBLE);
    constValue = makeDoubleConstant(currentToken->value);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredConstant(currentToken->string);
    if (obj->constAttrs->value->type == TP_INT ||
      obj->constAttrs->value->type == TP_DOUBLE ||
      obj->constAttrs->value->type == TP_CHAR ||
      obj->constAttrs->value->type == TP_STRING)
      constValue = duplicateConstantValue(obj->constAttrs->value);
    else
      error(ERR_UNDECLARED_NUMBER_CONSTANT, currentToken->lineNo,
        currentToken->colNo);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

Type* compileType(void) {
  Type* type = NULL;
  Type* elementType;
  int arraySize;
  Object* obj;

  switch (lookAhead->tokenType) {
  case KW_INTEGER:
    eat(KW_INTEGER);
    type = makeIntType();
    break;
  case KW_DOUBLE:
    eat(KW_DOUBLE);
    type = makeDoubleType();
    break;
  case KW_CHAR:
    eat(KW_CHAR);
    type = makeCharType();
    break;
  case KW_STRING:
    eat(KW_STRING);
    type = makeStringType();
    break;
  case KW_ARRAY:
    eat(KW_ARRAY);
    eat(SB_LSEL);
    eat(TK_NUMBER);

    arraySize = currentToken->value;

    eat(SB_RSEL);
    eat(KW_OF);
    elementType = compileType();
    type = makeArrayType(arraySize, elementType);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredType(currentToken->string);
    type = duplicateType(obj->typeAttrs->actualType);
    break;
  default:
    error(ERR_INVALID_TYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

Type* compileBasicType(void) {
  Type* type = NULL;

  switch (lookAhead->tokenType) {
  case KW_INTEGER:
    eat(KW_INTEGER);
    type = makeIntType();
    break;
  case KW_CHAR:
    eat(KW_CHAR);
    type = makeCharType();
    break;
  case KW_DOUBLE:
    eat(KW_DOUBLE);
    type = makeDoubleType();
    break;
  case KW_STRING:
    eat(KW_STRING);
    type = makeStringType();
    break;
  default:
    error(ERR_INVALID_BASICTYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

void compileParams(void) {
  if (lookAhead->tokenType == SB_LPAR) {
    eat(SB_LPAR);
    compileParam();
    while (lookAhead->tokenType == SB_SEMICOLON) {
      eat(SB_SEMICOLON);
      compileParam();
    }
    eat(SB_RPAR);
  }
}

void compileParam(void) {
  Object* param;
  Type* type;
  enum ParamKind paramKind = PARAM_VALUE;

  switch (lookAhead->tokenType) {
  case TK_IDENT:
    paramKind = PARAM_VALUE;
    break;
  case KW_VAR:
    eat(KW_VAR);
    paramKind = PARAM_REFERENCE;
    break;
  default:
    error(ERR_INVALID_PARAMETER, lookAhead->lineNo, lookAhead->colNo);
    break;
  }

  eat(TK_IDENT);
  checkFreshIdent(currentToken->string);
  param = createParameterObject(currentToken->string, paramKind,
    symtab->currentScope->owner);
  eat(SB_COLON);
  type = compileBasicType();
  param->paramAttrs->type = type;
  declareObject(param);
}

void compileStatements(void) {
  compileStatement();
  while (lookAhead->tokenType == SB_SEMICOLON) {
    eat(SB_SEMICOLON);
    compileStatement();
  }
}

void compileStatement(void) {
  switch (lookAhead->tokenType) {
  case TK_IDENT:
    compileAssignSt();
    break;
  case KW_CALL:
    compileCallSt();
    break;
  case KW_BEGIN:
    compileGroupSt();
    break;
  case KW_IF:
    compileIfSt();
    break;
  case KW_WHILE:
    compileWhileSt();
    break;
  case KW_DO:
    compileDoSt();
    break;
  case KW_FOR:
    compileForSt();
    break;
    // final term
    // thêm lệnh repeat until
    // repeat <statement> until <condition>
  case KW_REPEAT:
    compileRepeatSt();
    break;
    // thêm lệnh switch case
    // switch <expression>
    // BEGIN
    //  case <constant> : <statements>
    //  ...
    //  default : <statement>
    // END;
  case KW_SWITCH:
    compileSwitchSt();
    break;
  case KW_BREAK:
    compileBreakSt();
    break;
  case KW_SUM:
    compileSumSt();
    // Trong trường hợp statement rỗng
    // Token tiếp theo sẽ là follow của statement
    // Ví dụ như:
    // if x > 1 then else x := 1
    // Sau <then> đáng lẽ phải là 1 statement nhưng statement này rỗng
    // nên sau <then> sẽ là <else>
    // theo đúng thứ tự compileIfSt()
    // eat(IF)
    // compileCondition()
    // eat(THEN)
    // compileStatement()
    // eat(ELSE)
    // compileStatement()
    // tuy nhiên ở đây sau khi eat <THEN> vào compileStatement() sẽ gặp <ELSE>
    // Vậy hàm compileStatement() phải biết là nếu gặp <ELSE>
    // => statement rỗng và cho pass luôn.
    // Giả sử hàm compileStatement() này không có đoạn check follow sau đây
    // thì với trường hợp gặp <ELSE> như trên sẽ vào default và báo lỗi
    // Đó là lý do phải check folow của những thứ có thể rỗng
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_UNTIL:
  case KW_CASE:
  case KW_DEFAULT:
    break;
    // Error occurs
  default:
    error(ERR_INVALID_STATEMENT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

Type* compileLValue(void) {
  // parse a lvalue (a variable, an array element, a parameter, the current
  // function identifier)
  Object* var;
  Type* varType;

  eat(TK_IDENT);
  // check if the identifier is a function identifier, or a variable identifier,
  // or a parameter

  var = checkDeclaredLValueIdent(currentToken->string);

  if (var->kind == OBJ_VARIABLE) {
    if (var->varAttrs->type->typeClass == TP_ARRAY)
      varType = compileIndexes(var->varAttrs->type);
    else
      varType = duplicateType(var->varAttrs->type);
  }
  else if (var->kind == OBJ_PARAMETER) {
    varType = duplicateType(var->paramAttrs->type);
  }
  else {
    varType = duplicateType(var->funcAttrs->returnType);
  }

  return varType;
}

// final term
// compile toàn bộ vế trái trong phép gán
int compileLeftAssign(Type** LAssign, int top) {
  // giới hạn của phép gán nhiều biến là max assign
  // phải kiếm tra top hiện tại đã quá max assign chưa
  // nếu vượt quá giới hạn thì tạo ra lỗi
  if (top == MAX_ASSIGN) {
    error(ERR_TOO_MANY_ASSIGN, lookAhead->lineNo, lookAhead->colNo);
  }
  // Vì vế trái của phép gán nên sẽ là compile left value
  // compile left value tức là kiểu tra object hiện tại có được nằm bên trái ko
  // Ví dụ:
  // x := 1; x là biến nên x được nằm bên trái phép gán
  // 1 := 2; 1 là số nên 1 không được nằm bên trái
  // Kiểu của left value sẽ được lưu vào LAssign[top]
  // hiện tại top = 0
  // với phép gán nhiều biến sẽ gọi đệ quy hàm này với top tăng dần
  // Ví dụ như:
  // x, y, z = a, b, c;
  // Kiểu của x sẽ được lưu vào LAssign[0]
  // Kiểu của y sẽ được lưu vào LAssign[1]
  // ...
  // Tương tự với compileRAssign()
  // kiểu của a sẽ được lưu vào RAssign[0]
  // kiểu của b sẽ được lưu vào RAssign[1]
  // ...
  // sau đó ta so sánh kiểu của RAssign[0] với LAssign[0]
  // RAssign[1] với LAssign[1]
  // ...
  // từ đó sẽ check được phép gán có hợp lệ hay ko?
  LAssign[top] = compileLValue();

  switch (lookAhead->tokenType) {
  case SB_COMMA:
    eat(SB_COMMA);
    // Nếu gặp dấu phẩy tức đây là phép gán nhiều biến
    // gọi đệ quy với top + 1
    return compileLeftAssign(LAssign, top + 1);
    break;
    // Nếu gặp dấu := hoặc ::= tức là đã kết thúc vế trái
  case SB_ASSIGN:
  case SB_ASSIGN_2:
    return top + 1;
    break;
  default:
    DISABLE_RETURN_WARNING
      // Nếu không gặp dấu phẩy và cũng không gặp dấu := hoặc ::=
      // phép gán này sai cú pháp
      error(ERR_INVALID_STATEMENT, currentToken->lineNo, currentToken->colNo);
    break;
  }
}

// final term
// compile toàn bộ vế phải trong phép gán nhiều biến và gán bằng toán tử 3 ngôi
// phép gán nhiều biến:
//   ví dụ: a, b, c = 1, 2, 3
//   Định nghĩa:
//   a, b, c = <expression>, <expression>, <expression>
// phép gán toán tử 3 ngôi:
//   ví dụ: x = a > b ? c : d;
//   Định nghĩa:
//   x = <expression> <dấu so sánh> <expression> ? <expression> : <expression>

int compileRightAssign(Type** RAssign, int top) {
  // giới hạn của phép gán nhiều biến là max assign
  // phải kiếm tra top hiện tại đã quá max assign chưa
  // nếu vượt quá giới hạn thì tạo ra lỗi
  if (top == MAX_ASSIGN) {
    error(ERR_TOO_MANY_ASSIGN, lookAhead->lineNo, lookAhead->colNo);
  }

  // compile Expression đầu tiên xuất hiện ở cả 2 phép gán
  // kiểu của expression trả về sẽ lưu vào RAssign[top]
  // hiện tại top là 0
  // Trong phép gán nhiều biến sẽ gọi đệ quy hàm này với top tăng dần
  // Vì vậy trong phép gán nhiều biến:
  // a, b, c = <expression>, <expression>, <expression>
  // kiểu của các expression sẽ được lưu lần lượt vào RAssign[0], RAssign[1]...
  // sau đó ta so sánh kiểu của a với RAssign[0] và so sánh kiểu của b với
  // RAssign[1]...
  // kiểu của a, b, ... sẽ được lưu trong LAssign[0], LAssign[1]...
  // xem thêm hàm compileLeftAssign()
  RAssign[top] = compileExpression();

  switch (lookAhead->tokenType) {
    // Sau khi compile xong Expression đầu tiên
    // Nếu gặp dấu phẩy tức đây là phép gán nhiều biến
    // gọi là đệ quy hàm này với top tăng dần
  case SB_COMMA:
    eat(SB_COMMA);
    return compileRightAssign(RAssign, top + 1);
    break;
    // Nếu gặp các phép so sánh
    // tức đây là phép gán toán tử 3 ngôi
    // eat toán tử so sánh sau đó break ra ngoài
  case SB_EQ:
    eat(SB_EQ);
    break;
  case SB_NEQ:
    eat(SB_NEQ);
    break;
  case SB_LE:
    eat(SB_LE);
    break;
  case SB_LT:
    eat(SB_LT);
    break;
  case SB_GE:
    eat(SB_GE);
    break;
  case SB_GT:
    eat(SB_GT);
    break;

    // Nếu không gặp dấu phẩy hoặc các toán tử so sánh
    // => đây là cuối của vế phải
    // trả về top + 1 tức là số phần tử của vế phải
    // do bắt đầu đếm từ 0 nên phải trả về + 1
  default:
    return top + 1;
  }

  // Nếu gặp toán tử so sánh trong vế phải sẽ break ra đoạn này

  // compile <expression2> và lưu vào RType
  Type* RType = compileExpression();

  // Rtype và RAssign[top] đang chứa type của 2 vế của phép so sánh
  // RAssign[top] chứa type của vế trái phép so sánh (câu lệnh đầu tiên)
  // kiểm tra điều kiện type 2 vế của phép so sánh
  // hoặc cùng là số (int, double)
  // hoặc cùng là 1 kiểu nào đó
  if (RAssign[top]->typeClass == TP_INT ||
    RAssign[top]->typeClass == TP_DOUBLE) {
    checkNumberType(RType);
  }
  else {
    checkTypeEquality(RType, RAssign[top]);
  }

  eat(SB_QUESTION);  // sau phép so sánh là dấu ?

  // phép gán toán tử 3 ngôi:
  // x := a <op> b ? c : d
  // sau khi eat SB_QUESTION thì sẽ đến đoạn compile c và d
  // type của c và d sẽ gán trực tiếp vào
  // RAssign[top] để so sánh với LAssign[top]

  // Compile <expression3> và lưu vào LType
  // expression3 là giá trị sẽ được gán vào x nếu điều kiện đúng
  // tức là c ở trong ví dụ trên
  Type* LType = compileExpression();

  eat(SB_COLON);  // sau expression3 là dấu :

  // compile <expression4> và lưu vào RType
  // expression4 là giá trị sẽ được gán vào x nếu điều kiện sai
  // tức là d ở trong ví dụ trên
  RType = compileExpression();

  // RAssign[top] là kiểu của vế trái
  // so sánh RAssign[top] với kiểu của x để kiểm tra 2 vế của phép gán có-
  // -hợp lệ không
  // ví dụ: x = a > b ? 1: 2;
  // tức là x sẽ bằng 1 nếu điều kiện đúng
  // x bằng 2 nếu điều kiện sai
  // vậy phải kiếm tra x có là số nguyên hay không
  // tức là RAssign[top] sẽ phải mang kiểu int
  // để so sánh với LAssign[top] mang kiểu của x
  if (LType->typeClass == TP_INT || LType->typeClass == TP_DOUBLE) {
    // Nếu LType là kiểu số, RAssign[top] sẽ mang kiểu số
    // Nếu LType hoặc RType mang giá trị double thì RAssign[top] sẽ là double
    // Nếu cả 2 là int thì RAssign[top] sẽ là int
    RAssign[top] = autoUpcasting(LType, RType);
    ;
  }
  else {
    // Nếu LType và RType là kiểu không phải kiểu số
    // thì type của LType và RType sẽ phải giống nhau và giống RAssign[top]
    checkTypeEquality(LType, RType);
    RAssign[top] = duplicateType(RType);
  }
  // đã kiếm tra xong phép gán 3 ngôi
  // Nếu token tiếp theo là dấu , tức là còn phép gán nhiều biến
  // Nếu token tiếp theo khác , tức là kết thúc phép gán

  if (lookAhead->tokenType == SB_COMMA) {
    eat(SB_COMMA);
    return compileRightAssign(RAssign, top + 1);
  }
  else {
    return top + 1;
  }
}

// final term
// phép gán sẽ có 3 phép gán
// - gán có if, else: bên trái phép gán sẽ xử lý trực tiếp bên trong
// compileAssginSt()
// - gán nhiều biến: x, y := a, b;
// - gán bằng toán tử 3 ngôi: x := a > b ? c : d;
void compileAssignSt(void) {
  //  parse the assignment and check type consistency
  Type** LAssign = (Type**)calloc(MAX_ASSIGN, sizeof(Type*));
  Type** RAssign = (Type**)calloc(MAX_ASSIGN, sizeof(Type*));
  if (LAssign == NULL || RAssign == NULL) {
    printf("Error when calloc!");
    exit(1);
  }

  int Lvar = compileLeftAssign(LAssign, 0);

  // kiểm tra nếu sau vế trái là := tức đây là gán nhiều biến hoặc gán bằng toán
  // tử 3 ngôi
  if (lookAhead->tokenType == SB_ASSIGN) {
    eat(SB_ASSIGN);  // ngay sau vế trái sẽ là :=

    // compile toàn bộ vế phải bằng hàm này
    int Rvar = compileRightAssign(RAssign, 0);

    if (Lvar < Rvar) {
      error(ERR_ASSIGN_LEFT_LESS, currentToken->lineNo, currentToken->colNo);
    }
    else if (Lvar > Rvar) {
      error(ERR_ASSIGN_LEFT_MORE, currentToken->lineNo, currentToken->colNo);
    }

    for (int i = 0; i < Lvar; i++) {
      if (LAssign[i]->typeClass == TP_DOUBLE) {
        checkNumberType(RAssign[i]);
      }
      else {
        checkTypeEquality(RAssign[i], LAssign[i]);
      }
    }

  }
  // Nếu sau vế trái là ::= tức đây là gán if, else
  // x ::= if a > 0 return b else return c;
  // x = b nếu điều kiện đúng, bằng c nếu điều kiện sai
  // Định nghĩa phép gán:
  //    indent ::= if <condition> return <expression> else return <expression>;
  // ident: định danh (giống như tên biến)
  // condition: điều kiện
  // expression: biểu thức (giống như 1 + 2, 1 * 2 + 3, ...)
  else {
    eat(SB_ASSIGN_2);  // Ngay sau vế trái sẽ là ::=

    // vì gán bằng if, else nên vế trái chỉ có 1 biến, nếu có 2 biến tức là phép
    // gán sai cú pháp
    if (Lvar > 1) {
      error(ERR_ASSIGN_LEFT_MORE, currentToken->lineNo, currentToken->colNo);
    }

    eat(KW_IF);  // kiểm tra ngay sau dấu bằng có là Keyword IF hay không

    compileCondition();  // Ngay say Keyword IF là điều kiện nên gọi đến hàm này

    eat(KW_RETURN);  // Sau điều kiện là return

    Type* type1 =
      compileExpression();  // sau return là expression nên gọi đến hàm này
    eat(KW_ELSE);             // sau expression là else
    eat(KW_RETURN);           // sau else là return
    Type* type2 =
      compileExpression();  // sau return là expression nên gọi đến hàm này

  // Nếu vế trái của phép gán là double thì vế phải là số gì cũng được
    if (LAssign[0]->typeClass == TP_DOUBLE) {
      checkNumberType(type1);
      checkNumberType(type2);
    }
    // Nếu không là double thì vế phải là cùng định dạng
    else {
      checkTypeEquality(type1, LAssign[0]);
      checkTypeEquality(type2, LAssign[0]);
    }
  }
  //
  if (LAssign != NULL)
    free(LAssign);
  if (RAssign != NULL)
    free(RAssign);
}
// end assgin

void compileCallSt(void) {
  Object* proc;

  eat(KW_CALL);
  eat(TK_IDENT);

  proc = checkDeclaredProcedure(currentToken->string);

  compileArguments(proc->procAttrs->paramList);
}

void compileGroupSt(void) {
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileIfSt(void) {
  eat(KW_IF);
  compileCondition();
  eat(KW_THEN);
  compileStatement();
  if (lookAhead->tokenType == KW_ELSE)
    compileElseSt();
}

void compileElseSt(void) {
  eat(KW_ELSE);
  compileStatement();
}

void compileWhileSt(void) {
  eat(KW_WHILE);
  compileCondition();
  eat(KW_DO);
  compileStatement();
}

void compileDoSt(void) {
  eat(KW_DO);
  compileStatement();
  eat(KW_WHILE);
  compileCondition();
}

// final term
// thêm câu lệnh repeat until
// repeat <statement> until <condition>
void compileRepeatSt(void) {
  eat(KW_REPEAT);
  compileStatement();
  eat(KW_UNTIL);
  compileCondition();
}

// final term
// thêm câu lệnh switch case
// switch <expression>
// BEGIN
//  case <constant> : <statements>
//  case <constant> : <statements>
//  ...
//  default : <statement>
// END
void compileSwitchSt(void) {
  eat(KW_SWITCH);
  compileExpression();
  eat(KW_BEGIN);
  while (lookAhead->tokenType == KW_CASE) {
    eat(KW_CASE);
    compileConstant();
    eat(SB_COLON);
    compileStatements();
  }
  if (lookAhead->tokenType == KW_DEFAULT) {
    eat(KW_DEFAULT);
    eat(SB_COLON);
    compileStatement();
  }
  eat(KW_END);
}

// final term
// compile break
// do lệnh break nằm bên trong statements nên coi nó như 1 statement
void compileBreakSt(void) {
  eat(KW_BREAK);
}

// final term 
// thêm câu lệnh sum
// sum <expression>, <expression>, ...
// các expression đều phải trả về số
// sum "Hello", 1 + 2 là sai cú pháp
void compileSumSt(void) {
  Type* type;
  eat(KW_SUM);
  type = compileExpression();
  checkNumberType(type);
  while (lookAhead->tokenType == SB_COMMA) {
    eat(SB_COMMA);
    type = compileExpression();
    checkNumberType(type);
  }
}

void compileForSt(void) {
  // TCheck type consistency of FOR's variable
  Object* idex = NULL;
  eat(KW_FOR);
  eat(TK_IDENT);

  // check if the identifier is a variable
  idex = checkDeclaredVariable(currentToken->string);

  eat(SB_ASSIGN);
  Type* exp1 = compileExpression();
  checkTypeEquality(exp1, idex->varAttrs->type);

  eat(KW_TO);
  Type* exp2 = compileExpression();
  checkTypeEquality(exp2, idex->varAttrs->type);

  // final term
  // kiểm tra trong vòng for
  // for (int i = 0; i < 10; i++)
  // i có phải là số nguyên không?
  // Nếu i không phải là số nguyên thì báo lỗi
  if (idex->varAttrs->type->typeClass != TP_INT) {
    error(ERR_FOR_NOT_INT_INDEX, currentToken->lineNo, currentToken->colNo);
  }
  eat(KW_DO);
  compileStatement();
}

void compileArgument(Object* param) {
  // parse an argument, and check type consistency
  //       If the corresponding parameter is a reference, the argument must be
  //       a lvalue
  Type* expType;
  if (param == NULL) {
    error(ERR_INVALID_PARAMETER, currentToken->lineNo, currentToken->colNo);
  }

  if (param->paramAttrs->kind == PARAM_VALUE) {
    expType = compileExpression();
    checkTypeEquality(expType, param->paramAttrs->type);
  }
  else if (param->paramAttrs->kind == PARAM_REFERENCE) {
    expType = compileLValue();
    checkTypeEquality(expType, param->paramAttrs->type);
  }
}

void compileArguments(ObjectNode* paramList) {
  // parse a list of arguments, check the consistency of the arguments and the
  // given parameters
  Object* param;
  ObjectNode* root = paramList;
  if (root == NULL) {
    param = NULL;
  }
  else {
    param = root->object;
    root = root->next;
  }

  switch (lookAhead->tokenType) {
  case SB_LPAR:
    eat(SB_LPAR);
    compileArgument(param);

    while (lookAhead->tokenType == SB_COMMA) {
      eat(SB_COMMA);
      if (root != NULL) {
        param = root->object;
        root = root->next;
      }
      else {
        param = NULL;
      }
      compileArgument(param);
    }

    eat(SB_RPAR);
    break;
    // Check FOLLOW set
  case SB_TIMES:
  case SB_SLASH:
    // final term
    // Thêm phép toán module
    // Vì module có mức ưu tiên như phép nhân và phép chia
    // ctrl + f và tìm kiếm SB_SLASH (phép chia) hoặc SB_TIMES (phép nhân)
    // và điền SB_MOD tương ứng vào là được
  case SB_MOD:
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case SB_QUESTION:
  case SB_COLON:
  case KW_END:
  case KW_ELSE:
  case KW_RETURN:
  case KW_THEN:
  case KW_WHILE:
  case KW_UNTIL:
    break;
  default:
    error(ERR_INVALID_ARGUMENTS, lookAhead->lineNo, lookAhead->colNo);
  }
}

void compileCondition(void) {
  // check the type consistency of LHS and RSH, check the basic type
  Type* LType = compileExpression();

  switch (lookAhead->tokenType) {
  case SB_EQ:
    eat(SB_EQ);
    break;
  case SB_NEQ:
    eat(SB_NEQ);
    break;
  case SB_LE:
    eat(SB_LE);
    break;
  case SB_LT:
    eat(SB_LT);
    break;
  case SB_GE:
    eat(SB_GE);
    break;
  case SB_GT:
    eat(SB_GT);
    break;
  default:
    error(ERR_INVALID_COMPARATOR, lookAhead->lineNo, lookAhead->colNo);
  }

  Type* RType = compileExpression();

  if (LType->typeClass == TP_INT || LType->typeClass == TP_DOUBLE) {
    checkNumberType(RType);
  }
  else {
    checkTypeEquality(RType, LType);
  }
}

// final term
// thêm phép cộng cho chuỗi
// định nghĩa: "hello" + " world" = "hello world"
// phép cộng trong KPL:
// các phép cộng trừ trong KPL được xử lý bằng Expression
// Expression: dấu cộng, trừ ở phía trước phép tính
//    ví dụ: -1 + 2 + 3 - 4
//    => dấu - trước số 1 sẽ được xử lý trong expression
// Expression2: Nhân tử đầu tiên và toàn bộ phép tính còn lại
//    ví dụ: -1 + 2 + 3 - 4
//    => expression2 sẽ chia phép tính thành:
//      phần 1: -1
//      phần 2: + 2 + 3 - 4
// Tại sao lại cần chia ra xử lý riêng nhân tử đầu tiên:
// Vì trong trường hợp không có phép cộng trừ nào thì phần 2 sẽ rỗng
// và chỉ cần phần 1, lúc này coi như 1 phép gán thông thường
// Expression3: Phép cộng trừ trong phép tính
//    ví dụ: -1 + 2 + 3 - 4
//    => + 2, +3, -4 sẽ được xử lý trong expression3
// expression3 cũng có thể rỗng
// để xử lý trường hợp không có phép cộng và không kéo dài phép cộng
//
// Để xử lý phép cộng String
// tức là "hello" + " world"
// "hello" là nhân tử đầu tiên nên sẽ phải xử lý trong expression2
// + "world" sẽ phải xử lý trong expression3
// hàm expression này sẽ giữ nguyên

Type* compileExpression(void) {
  Type* type;

  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    type = compileExpression2();
    checkNumberType(type);
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    type = compileExpression2();
    checkNumberType(type);
    break;
  default:
    type = compileExpression2();
  }
  return type;
}

// hàm expression2 xử lý nhân tử đầu tiên và toàn bộ phép cộng trừ phía sau
// expression2 := term expression3
// term là nhân tử đầu tiên và expression3 là toàn bộ phép cộng trừ
// Nếu expression3 là rỗng, ko cần quan tâm đến type của term
// vì coi như đây chỉ là phép gán bình thường
// x = a;
// Nếu expression3 khác rỗng
// ban đầu là sẽ kiểm tra type của term và expression3 có phải là số hay ko
// bây h cho phép cộng String rồi nên phải kiểm tra thêm trường hợp String nữa
Type* compileExpression2(void) {
  Type* type1;
  Type* type2;

  type1 = compileTerm();
  type2 = compileExpression3();
  if (type2 == NULL)
    return type1;
  else {
    if (type2->typeClass == TP_STRING && type1->typeClass == TP_STRING) {
      return type2;
      // hoặc return type 1 cũng được
    }
    else {
      return autoUpcasting(type1, type2);
    }
  }
}

Type* compileExpression3(void) {
  Type* type1;
  Type* type2;

  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    type1 = compileTerm();
    // trước đó là checkNumberType(type1);
    // bây h sẽ cho phép cả String
    if (type1 == NULL ||
      (type1->typeClass != TP_DOUBLE && type1->typeClass != TP_INT &&
        type1->typeClass != TP_STRING))
      error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo,
        currentToken->colNo);

    type2 = compileExpression3();
    if (type2 != NULL) {
      if (type2->typeClass == TP_STRING && type1->typeClass == TP_STRING) {
        return type2;
        // hoặc return type1 cũng được
      }
      if (type2->typeClass == TP_INT || type2->typeClass == TP_DOUBLE) {
        return autoUpcasting(type1, type2);
      }
    }

    return type1;
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    type1 = compileTerm();
    checkNumberType(type1);

    type2 = compileExpression3();
    if (type2 != NULL) {
      return autoUpcasting(type1, type2);
    }

    return type1;
    break;
    // check the FOLLOW set
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case SB_QUESTION:
  case SB_COLON:
  case KW_END:
  case KW_ELSE:
  case KW_RETURN:
  case KW_THEN:
  case KW_WHILE:
  case KW_UNTIL:
  case KW_BEGIN:
    return NULL;
    break;
  default:
    error(ERR_INVALID_EXPRESSION, lookAhead->lineNo, lookAhead->colNo);
  }
  return NULL;
}

// final term
// thêm phép toán power
// định nghĩa: 2**n = 2 ^ n;
// Vì phép toán power có ưu tiên cao hơn phép toán nhân chia
// nên phải thêm kí hiệu không kết thúc vào sản xuất của KPL
// Ban đầu:
//      Term ::= Factor Term2
//      Term2 ::= SB_TIMES Factor Term2
//      Term2 ::= SB_SLASH Factor Term2
//      Term2 ::= épsilon
// Sẽ đổi thành:
//      Term ::= Power Term2
//      Term2 ::= SB_TIMES Power Term2
//      Term2 ::= SB_SLASH Power Term2
//      Term2 ::= épsilon
//      Power ::= Factor Power2
//      Power2 ::= SB_POWER Factor Power2
//      Power2 ::= épsilon
//
// thêm 2 hàm compilePower và compilePower2
// sửa đổi trong compileTerm2 và compileTerm theo như sản xuất lúc sau
//
// Có thể bé chưa biết:
// Tại sao ưu tiên cao hơn thì lại thêm kí hiệu ko kết thúc?
// hãy nhìn vào sản xuất của KPL:
// expression là các phép cộng, trừ, ưu tiên thấp nhất
// term là nhân chia, ưu tiên cao hơn công trừ
// power là phép lấy mũ, ưu tiên cao hơn nhân chia
//  -  term là do expression sản xuất ra
//  Vì expression2 = term + expression3
//  => power là do term sản xuất ra
//  => term = power + term2;
Type* compileTerm(void) {
  Type* type1;
  Type* type2;

  // ban đầu là type1 = compileFactor()
  type1 = compilePower();
  type2 = compileTerm2();

  if (type2 != NULL) {
    return autoUpcasting(type1, type2);
  }

  return type1;
}

Type* compileTerm2(void) {
  Type* type1;
  Type* type2;

  switch (lookAhead->tokenType) {
  case SB_TIMES:
    eat(SB_TIMES);
    // ban đầu type1 = compileFactor()
    type1 = compilePower();
    checkNumberType(type1);

    type2 = compileTerm2();
    if (type2 != NULL) {
      return autoUpcasting(type1, type2);
    }
    return type1;
    break;
  case SB_SLASH:
    eat(SB_SLASH);
    // ban đầu type1 = compileFactor()
    type1 = compilePower();
    checkNumberType(type1);

    type2 = compileTerm2();
    if (type2 != NULL) {
      return autoUpcasting(type1, type2);
    }
    return type1;
    break;
    // final term
    // thêm phép toán module:
    // định nghĩa: 5 % 3 = 2;
    // phép toán module tương đương với phép nhân chia
    // copy đoạn trên xuống rồi sửa SB_SLASH thành SB_MOD
  case SB_MOD:
    eat(SB_MOD);
    // ban đầu type1 = compileFactor()
    type1 = compilePower();
    checkNumberType(type1);

    type2 = compileTerm2();
    if (type2 != NULL) {
      return autoUpcasting(type1, type2);
    }
    return type1;
    break;
    // check the FOLLOW set
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case SB_QUESTION:
  case SB_COLON:
  case KW_END:
  case KW_ELSE:
  case KW_RETURN:
  case KW_THEN:
  case KW_WHILE:
  case KW_UNTIL:
  case KW_BEGIN:
    break;
  default:
    error(ERR_INVALID_TERM, lookAhead->lineNo, lookAhead->colNo);
  }
  return NULL;
}
// final term compilePower và compilePower2
// compile power và compile power2 tuân theo cú pháp của KPL sửa đổi
// cú pháp kpl sửa đổi nằm ở comment phía trên
Type* compilePower(void) {
  Type* type1;
  Type* type2;

  type1 = compileFactor();
  type2 = compilePower2();

  if (type2 != NULL) {
    return autoUpcasting(type1, type2);
  }
  return type1;
}

Type* compilePower2(void) {
  Type* type1;
  Type* type2;

  switch (lookAhead->tokenType) {
  case SB_POWER:
    eat(SB_POWER);
    type1 = compileFactor();
    checkNumberType(type1);

    type2 = compilePower2();
    if (type2 != NULL) {
      return autoUpcasting(type1, type2);
    }
    return type1;
    break;
    // check the FOLLOW set
  case SB_PLUS:
  case SB_MINUS:
  case SB_TIMES:
  case SB_SLASH:
  case SB_MOD:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case SB_QUESTION:
  case SB_COLON:
  case KW_END:
  case KW_ELSE:
  case KW_RETURN:
  case KW_THEN:
  case KW_WHILE:
  case KW_UNTIL:
  case KW_BEGIN:
    break;
  default:
    error(ERR_INVALID_TERM, lookAhead->lineNo, lookAhead->colNo);
  }
  return NULL;
}

// compile factor không sửa đổi gì
Type* compileFactor(void) {
  // parse a factor and return the factor's type

  Object* obj;
  Type* type = NULL;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    type = makeIntType();
    break;
  case TK_DOUBLE:
    eat(TK_DOUBLE);
    type = makeDoubleType();
    break;
  case TK_STRING:
    eat(TK_STRING);
    type = makeStringType();
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    type = makeCharType();
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    // check if the identifier is declared

    obj = checkDeclaredIdent(currentToken->string);

    switch (obj->kind) {
    case OBJ_CONSTANT:
      if (obj->constAttrs->value->type == TP_INT) {
        type = makeIntType();
      }
      else if (obj->constAttrs->value->type == TP_DOUBLE) {
        type = makeDoubleType();
      }
      else if (obj->constAttrs->value->type == TP_CHAR) {
        type = makeCharType();
      }
      else if (obj->constAttrs->value->type == TP_STRING) {
        type = makeStringType();
      }
      else {
        error(ERR_INVALID_CONSTANT, currentToken->lineNo,
          currentToken->colNo);
      }
      break;
    case OBJ_VARIABLE:
      if (obj->varAttrs->type->typeClass == TP_ARRAY) {
        type = compileIndexes(obj->varAttrs->type);
      }
      else {
        type = duplicateType(obj->varAttrs->type);
      }
      break;
    case OBJ_PARAMETER:
      type = duplicateType(obj->paramAttrs->type);
      break;
    case OBJ_FUNCTION:
      compileArguments(obj->funcAttrs->paramList);
      type = duplicateType(obj->funcAttrs->returnType);
      break;
    default:
      error(ERR_INVALID_FACTOR, currentToken->lineNo, currentToken->colNo);
      break;
    }
    break;
  default:
    error(ERR_INVALID_FACTOR, lookAhead->lineNo, lookAhead->colNo);
  }

  return type;
}

Type* compileIndexes(Type* arrayType) {
  // parse a sequence of indexes, check the consistency to the arrayType, and
  // return the element type
  Type* expType;
  while (lookAhead->tokenType == SB_LSEL) {
    eat(SB_LSEL);
    expType = compileExpression();
    checkIntType(expType);
    if (arrayType->typeClass == TP_ARRAY)
      arrayType = arrayType->elementType;
    else
      error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
    eat(SB_RSEL);
  }
  return arrayType;
  // final term
  // arrayType trả về có thể là TP_ARRAY hoặc TP_INT hoặc TP_DOUBLE
  // hoặc TP_CHAR hoặc TP_STRING
  // Nếu muốn phép gán cho phép gán mảng thì chỉ cần return arrayType
  // Nếu muốn phép gán ko cho phép gán mảng cần kiểm tra arrayType
  // Nếu arrayType là TP_ARRAY thì báo lỗi
  // if (arrayType->typeClass != TP_ARRAY) {
  //   return arrayType;
  // } else {
  //   error(ERR_TYPE_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
  // }
}

int compile(char* fileName) {
  if (openInputStream(fileName) == IO_ERROR)
    return IO_ERROR;

  currentToken = NULL;

  lookAhead = getValidToken();

  initSymTab();
  compileProgram();

  printObject(symtab->program, 0);

  cleanSymTab();

  free(currentToken);
  free(lookAhead);
  closeInputStream();
  return IO_SUCCESS;
}