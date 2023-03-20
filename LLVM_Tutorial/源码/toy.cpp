#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

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
    PrototypeAST(const std::string &name, std::vector<std::string> &args) : Name(name), Args(args) {}
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