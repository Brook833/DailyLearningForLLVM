
#include <map>
#include <string>
#include <vector>
#include <iostream>

//===----------------------------------------------------------------------===//
// 词法分析器
//===----------------------------------------------------------------------===//

// 如果是一个未知的字符，词法法分析器返回词元[0-255]
enum Token {
    tok_eof = -1,

    tok_def = -2,
    tok_extern = -3,

    tok_identifier = -4,
    tok_number = -5,
};

static std::string IdentifierStr;  // 标识符
static double NumVal;              // 数值

// gettok - 从标准输入中返回下一个词元
static int gettok() {
    static int LastChar = ' ';

    // 跳过空字符
    while (isspace(LastChar)) {
        LastChar = getchar();
    }

    // 标识符(tok_identifier) [a-z A-Z][a-z A-Z 0-9]
    if (isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while (isalnum(LastChar = getchar())) {
            IdentifierStr += LastChar;
        }
        if (IdentifierStr == "def") {
            return tok_def;
        }
        if (IdentifierStr == "extern") {
            return tok_extern;
        }
        return tok_identifier;
    }

    // 数字(tok_number) [0-9]
    if (isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while(isdigit(LastChar) || LastChar == '.');
        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }

    // 注释
    if (LastChar == '#') {
        do {
            LastChar = getchar();
        } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF) {
            return gettok();
        }
    }

    if (LastChar == EOF) {
        return tok_eof;
    }

    // 未知字符(操作符)，返回其ASCII值
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

//===----------------------------------------------------------------------===//
// 抽象语法树 (AST)
//===----------------------------------------------------------------------===//

// ExprAST - 所有表达式节点的基类
class ExprAST {
public:
    ~ExprAST() {}
};

// NumberExprAST - 数字文本的AST类，如"1.0"
class NumbeExprAST : public ExprAST {
    double Val;
public:
    NumbeExprAST(double val) : Val(val) {}
};

// VariableExprAST - 引用变量的AST类，如"a"
class VariableExprAST : public ExprAST {
    std::string Name;
public:
    VariableExprAST(const std::string &name) : Name(name) {}
};

// BinaryExprAST - 二元操作符的AST类，如"+"
class BinaryExprAST : public ExprAST {
    char Op;
    ExprAST *LHS, *RHS;
public:
    BinaryExprAST(char op, ExprAST *lhs, ExprAST *rhs) : Op(op), LHS(lhs), RHS(rhs) {}
};

// CallExprAST - 函数调用的AST类
class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<ExprAST *> Args;
public:
    CallExprAST(const std::string &callee, std::vector<ExprAST*> &args) : Callee(callee), Args(args) {}
};

// PrototypeAST - 此类表示函数的原型，它捕获函数名称和参数名称
class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;
public:
    PrototypeAST(const std::string &name, const std::vector<std::string> &args) : Name(name), Args(args) {}
};

// FunctionAST - 此类表示函数的定义，它捕获函数原型及其函数体
class FunctionAST {
    PrototypeAST *Proto;
    ExprAST *Body;
public:
    FunctionAST(PrototypeAST *proto, ExprAST* body) : Proto(proto), Body(body) {}
};

//===----------------------------------------------------------------------===//
// 语法分析器
//===----------------------------------------------------------------------===//

// CurTok/getNextToken - 提供一个词元缓存
// CurTok - 语法分析器正在分析的词元
// getNextToken - 获取下一个词元并更新CurTok
static int CurTok;
static int getNextToken() {
    return CurTok = gettok();
}

// BinopPrecedence - 存储每个运算符的优先级
static std::map<char, int> BinopPrecedence;

// GetTokPrecedence - 获取操作符的优先级
static int GetTokPrecedence() {
    if (!isascii(CurTok)) {
        return -1;
    }

    int TokPrec = BinopPrecedence[CurTok];
    if (TokPrec <= 0) {
        return -1;
    }
    return TokPrec;
}

// Error - 用于错误处理的辅助函数
ExprAST *Error(const char *Str) {
    fprintf(stderr, "Error %s\n", Str);
    return 0;
}

PrototypeAST *ErrorP(const char *Str) {
    Error(Str);
    return 0;
}

FunctionAST *ErrorF(const char *Str) {
    Error(Str);
    return 0;
}

// 声明，ParseIdentifierExpr需要
static ExprAST *ParseExpression();

// ParseIdentifierExpr - 解析标识符
// ::= identifier
// ::= identifier '(' expression ')'
static ExprAST *ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;
    
    // 此时CurTok已经是identifier
    getNextToken();

    if (CurTok != '(') {  // 简单变量
        return new VariableExprAST(IdName);
    }

    // 此时CurTok已经是'('
    getNextToken();
    std::vector<ExprAST*> Args;
    if (CurTok != ')') {
        while(1) {
            ExprAST *Arg = ParseExpression();
            if (!Arg) {
                return 0;
            }
            Args.push_back(Arg);

            if (CurTok == ')') {
                break;
            }

            if (CurTok != ',') {
                return Error("参数里表中缺少','或')'");
            }
            getNextToken();
        }
    }

    getNextToken();
    return new CallExprAST(IdName, Args);
}

// ParseNumberExpr - 解析数字文本
static ExprAST *ParseNumberExpr() {
    ExprAST *Result = new NumbeExprAST(NumVal);
    getNextToken();
    return Result;
}

// ParseParenExpr - 解析'('expression')'
static ExprAST *ParseParenExpr() {
    getNextToken();
    ExprAST *V = ParseExpression();
    if (!V) {
        return 0;
    }

    if (CurTok != ')') {
        return Error("缺少括号");
    }

    getNextToken();
    return V;
}

// ParsePrimary - 根据词元解析
// ::=identifier
// ::=numberexpr
// ::=parenexpr
static ExprAST *ParsePrimary() {
    switch(CurTok) {
        default:
            return Error("解析表达式时出现未知的词元");
        case tok_identifier:
            return ParseIdentifierExpr();
        case tok_number:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
    }
}

// ParseBinOpRHS - 解析剩余表达式
// ::='(' + primary)
static ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS) {
    while(1) {
        int TokePrec = GetTokPrecedence();
        if (TokePrec < ExprPrec) {
            return LHS;
        }

        int BinOp = CurTok;
        getNextToken();

        ExprAST *RHS = ParsePrimary();
        if (!RHS) {
            return 0;
        }

        // 如果左运算符优先级小于右运算符优先级，则先去处理右边的部分
        int NextPrec = GetTokPrecedence();
        if (TokePrec < NextPrec) {
            RHS = ParseBinOpRHS(TokePrec + 1, RHS);
            if (!RHS) {
                return 0;
            }
        }
        
        // 合并LHS/RHS
        LHS = new BinaryExprAST(BinOp, LHS, RHS);
    }
}

// 解析最左表达式
// ::=primary binoprhs
static ExprAST *ParseExpression() {
    ExprAST *LHS = ParsePrimary();
    if (!LHS) {
        return 0;
    }
    
    return ParseBinOpRHS(0, LHS);
}

// ParsePrototype - 解析函数原型
// ::=id'('args')'
static PrototypeAST *ParsePrototype() {
    if (CurTok != tok_identifier) {
        return ErrorP("函数原型缺少函数名");
    }

    std::string FnName = IdentifierStr;
    getNextToken();

    if (CurTok != '(') {
        return ErrorP("函数原型中缺少'('");
    }

    std::vector<std::string> ArgNames;
    while(getNextToken() == tok_identifier) {
        ArgNames.push_back(IdentifierStr);
    }

    if (CurTok !=')') {
        return ErrorP("函数原型缺少')'");
    }

    getNextToken();
    
    return new PrototypeAST(FnName, ArgNames);
}

// ParseDefinition - 解析函数定义
static FunctionAST *ParseDefinition() {
    // def
    getNextToken();
    
    PrototypeAST *Proto = ParsePrototype();
    if (Proto == 0) {
        return 0;
    }

    if (ExprAST *E = ParseExpression()) {
        return new FunctionAST(Proto, E);
    }

    return 0;
}

// ParseTopLevelExpr - 解析顶层表达式
static FunctionAST *ParseTopLevelExpr() {
    if (ExprAST *E = ParseExpression()) {
        PrototypeAST *Proto = new PrototypeAST("", std::vector<std::string>());
        return new FunctionAST(Proto, E);
    }
    return 0;
}

// ParseExtern - 解析extern语句
static PrototypeAST *ParseExtern() {
    // extern
    getNextToken();
    return ParsePrototype();
}

//===----------------------------------------------------------------------===//
// 解析顶层语句
//===----------------------------------------------------------------------===//

// HandleDefinition - 处理定义
static void HandleDefinition() {
    if (ParseDefinition()) {
        fprintf(stderr, "解析一个函数定义\n");
    } else {
        // 跳过错误词元
        getNextToken();
    }
}

//
static void HandleExtern() {
    if (ParseExtern()) {
        fprintf(stderr, "解析一个extern\n");
    } else {
        getNextToken();
    }
}

// HandleTopLevelExpression - 处理顶层表达式(将顶层表达式看作匿名函数)
static void HandleTopLevelExpression() {
    if(ParseTopLevelExpr()) {
        fprintf(stderr, "解析一个顶层表达式\n");
    } else {
        getNextToken();
    }
}

// 顶层::=def/extern/expr/';'
static void MainLoop() {
    while(1) {
        fprintf(stderr, "ready> ");
        switch (CurTok) {
            case tok_eof:
                return;
            case ';':
                getNextToken();
                break;
            case tok_def:
                HandleDefinition();
                break;
            case tok_extern:
                HandleExtern();
                break;
            default:
                HandleTopLevelExpression();
                break;
        }
    }
}

int main() {
    // 值越大优先级越高
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;

    fprintf(stderr, "ready> ");
    getNextToken();

    MainLoop();

    return 0;
}