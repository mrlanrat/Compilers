/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */

#include <string.h>
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"

extern SymbolTable *symbols;

IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
}

Location* IntConstant::Emit(CodeGenerator* codeGen) {
  return codeGen->GenLoadConstant(value);
}

DoubleConstant::DoubleConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
}

Location* BoolConstant::Emit(CodeGenerator *codeGen) {
    return codeGen->GenLoadConstant(value ? 1 : 0);
}

StringConstant::StringConstant(yyltype loc, const char *val) : Expr(loc) {
    Assert(val != NULL);
    value = strdup(val);
}

Location* StringConstant::Emit(CodeGenerator *codeGen) {
    return codeGen->GenLoadConstant(value);
}

Location* NullConstant::Emit(CodeGenerator *codeGen) {
    return codeGen->GenLoadConstant(0);
}


Type* ArithmeticExpr::GetType() {
    return right->GetType();
}

int ArithmeticExpr::GetBytes() {
    if (left && right) return CodeGenerator::VarSize + left->GetBytes() + right->GetBytes();
    return (2 * CodeGenerator::VarSize) + right->GetBytes();
}

Location* ArithmeticExpr::Emit(CodeGenerator *codeGen) {
    if (left && right)
        return codeGen->GenBinaryOp(op->GetName(), left->Emit(codeGen), right->Emit(codeGen));

    // If the operator is '-' return 'right == false'
    return codeGen->GenBinaryOp("-", codeGen->GenLoadConstant(0), right->Emit(codeGen));
}

Type* RelationalExpr::GetType() {
    return Type::boolType;
}

int RelationalExpr::GetBytes() {
    int size = CodeGenerator::VarSize + left->GetBytes() + right->GetBytes(); 
    if (op->EqualTo("<="))
        return (2 * CodeGenerator::VarSize) + size;
    else if (op->EqualTo(">"))
        return (4 * CodeGenerator::VarSize) + size;
    else if (op->EqualTo(">="))
        return (2 * CodeGenerator::VarSize) + size;
    else
        return size;
}

Location* RelationalExpr::Emit(CodeGenerator *codeGen) {
    Location *l     = left->Emit(codeGen);
    Location *r     = right->Emit(codeGen);
    if (op->EqualTo("<"))
        return codeGen->GenBinaryOp("<", l, r);
    else if (op->EqualTo("<=")) {
        Location *equal = codeGen->GenBinaryOp("==", l, r);
        Location *less  = codeGen->GenBinaryOp("<", l, r);
        return codeGen->GenBinaryOp("||", less, equal);
    }
    else if (op->EqualTo(">")) {
        Location *equal = codeGen->GenBinaryOp("==", l, r);
        Location *less  = codeGen->GenBinaryOp("<", l, r);
        Location *leq   = codeGen->GenBinaryOp("||", less, equal);
        return codeGen->GenBinaryOp("==", codeGen->GenLoadConstant(0), leq);
    }
    else if (op->EqualTo(">=")) {
        Location *less  = codeGen->GenBinaryOp("<", l, r);
        return codeGen->GenBinaryOp("==", codeGen->GenLoadConstant(0), less);
    }

    return NULL;
}

Type* EqualityExpr::GetType() {
    return Type::boolType;
}

int EqualityExpr::GetBytes() {
    int size = CodeGenerator::VarSize + left->GetBytes() + right->GetBytes(); 
    if (op->EqualTo("=="))
        return size;
    return size + (2 * CodeGenerator::VarSize);
}

Location* EqualityExpr::Emit(CodeGenerator *codeGen) {
    Location *loc = codeGen->GenBinaryOp("==", left->Emit(codeGen), right->Emit(codeGen));
    if (op->EqualTo("=="))
        return loc;

    // If operator is '!=' reverse the result of '=='
    return codeGen->GenBinaryOp("==", codeGen->GenLoadConstant(0), loc);
}

Type* LogicalExpr::GetType() {
    return Type::boolType;
}

int LogicalExpr::GetBytes() {
    if (left && right) return CodeGenerator::VarSize + left->GetBytes() + right->GetBytes();
    return (2 * CodeGenerator::VarSize) + right->GetBytes();
}

Location* LogicalExpr::Emit(CodeGenerator *codeGen) {
    if (left && right)
        return codeGen->GenBinaryOp(op->GetName(), left->Emit(codeGen), right->Emit(codeGen));

    // If the operator is '!' return 'right == false'
    return codeGen->GenBinaryOp("==", codeGen->GenLoadConstant(0), right->Emit(codeGen));
}

Type* AssignExpr::GetType() {
    return left->GetType();
}

int AssignExpr::GetBytes() {
    return right->GetBytes();
}

Location* AssignExpr::Emit(CodeGenerator *codeGen) {
    Location *r = right->Emit(codeGen);
    codeGen->GenAssign(left->Emit(codeGen), r);
    return r;
}


Type* This::GetType() {
    //TODO
    cout << "This::GetType:TODO" << endl;
    return NULL;
}

Location* This::Emit(CodeGenerator *codeGen) {
  //TODO
  cout << "This::Emit:TODO" << endl;
  return NULL;
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r) 
  : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op=o)->SetParent(this);
    (left=l)->SetParent(this); 
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r) 
  : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL; 
    (op=o)->SetParent(this);
    (right=r)->SetParent(this);
}
   
int CompoundExpr::GetBytes() {
    if (left && right) return CodeGenerator::VarSize + left->GetBytes() + right->GetBytes();
    return CodeGenerator::VarSize + right->GetBytes();
}

Location* CompoundExpr::Emit(CodeGenerator *codeGen) {
    if (left && right)
        return codeGen->GenBinaryOp(op->GetName(), left->Emit(codeGen), right->Emit(codeGen));

    // TODO: Handle case of one operand.
    return NULL;
}

ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

Type* ArrayAccess::GetType() {
    return base->GetType()->GetType();
}

int ArrayAccess::GetBytes() {
    return CodeGenerator::VarSize;
}

Location* ArrayAccess::Emit(CodeGenerator *codeGen) {
    Location *varSize = codeGen->GenLoadConstant(CodeGenerator::VarSize);
    Location *offset = codeGen->GenBinaryOp("*", subscript->Emit(codeGen), varSize);
    Location *location = codeGen->GenBinaryOp("+", base->Emit(codeGen), offset);
    return codeGen->GenLoad(location); 
}


FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}

Type* FieldAccess::GetType() {
    Decl *decl = symbols->Search(field->GetName());
    
    return decl->GetType();
}

Location* FieldAccess::Emit(CodeGenerator *codeGen) {
  Decl *decl = symbols->Search(field->GetName());
  Location *loc = decl->GetLoc();
  return loc;
}


Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}
 

Type* Call::GetType() {
    if (!base) {
        return field->GetType();
    }
    else if (base->GetType()->IsArrayType()) {
        if (strcmp(field->GetName(), "length") == 0)
            return Type::intType;
    }
    return NULL;
}

int Call::GetBytes(){
    int n = 0;
    for (int i = 0; i < actuals->NumElements(); i++) {
        n += actuals->Nth(i)->GetBytes();
    }

    if (!base) {
        Decl *decl = symbols->Search(field->GetName());
        if (!decl->GetType()->IsEquivalentTo(Type::voidType)) {
            n += CodeGenerator::VarSize;
        }
    }
    return n;
}

Location* Call::Emit(CodeGenerator *codeGen) {
    Location *result = NULL;
    int bytes = 0;
    // Tac instructions push params right to left.
    for (int i = actuals->NumElements() - 1; i >= 0; i--) {
        bytes += CodeGenerator::VarSize;
        Location *loc = actuals->Nth(i)->Emit(codeGen);
        codeGen->GenPushParam(loc);
    }
    if (!base) {
        Decl *decl = symbols->Search(field->GetName());
        if (decl->GetType()->IsEquivalentTo(Type::voidType)) {
            result = codeGen->GenLCall(field->GetName(), false);
        }
        else {
            result = codeGen->GenLCall(field->GetName(), true);
        }
    }
    else if (base->GetType()->IsArrayType()) {
        /*
        char label[80];
        sprintf(label, "_%s_length", base->GetName());
        if (strcmp(field->GetName(), "length") == 0) {
            result = codeGen->GenLCall(label, true);
        }
        */
        result = codeGen->GenLoadConstant(0);
    }
    codeGen->GenPopParams(bytes);
    return result;
}

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
  Assert(c != NULL);
  (cType=c)->SetParent(this);
}

Type* NewExpr::GetType() {
    //TODO
    cout << "NewExpr::Type:TODO" << endl;
    return NULL;
}


Location* NewExpr::Emit(CodeGenerator *codeGen) {
  //TODO
  cout << "NewExpr::Emit:TODO" << endl;
  return NULL;
}


//TODO not sure if this is right
int NewExpr::GetBytes() {
    return 5 * CodeGenerator::VarSize;
}

NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this); 
    (elemType=et)->SetParent(this);
}

Type* NewArrayExpr::GetType() {
    return elemType;
}

int NewArrayExpr::GetBytes() {
    return CodeGenerator::VarSize;
}

Location* NewArrayExpr::Emit(CodeGenerator *codeGen) {
    return codeGen->GenBuiltInCall(Alloc, size->Emit(codeGen));
}


Type* ReadIntegerExpr::GetType() {
    return Type::intType;
}
Location* ReadIntegerExpr::Emit(CodeGenerator *codeGen) {
    return codeGen->GenBuiltInCall(ReadInteger);
}

Type* ReadLineExpr::GetType() {
    return Type::stringType;
}
Location* ReadLineExpr::Emit(CodeGenerator *codeGen) {
    return codeGen->GenBuiltInCall(ReadLine);
}
