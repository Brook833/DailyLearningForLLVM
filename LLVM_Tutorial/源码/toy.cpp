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

    // 未知字符，返回其ASCII值
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