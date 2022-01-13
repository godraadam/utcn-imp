// This file is part of the IMP project.

#pragma once

#include <vector>
#include <memory>
#include <variant>
#include <iostream>
#include <string>
/**
 * Base class for all AST nodes.
 */
class Node
{
};

/**
 * Base class for all statements.
 */
class Stmt : public Node
{
public:
    enum class Kind
    {
        BLOCK,
        WHILE,
        EXPR,
        RETURN,
        VARDECL,
        IF,
        ELSE
    };

public:
    Kind GetKind() const { return kind_; }

protected:
    Stmt(Kind kind) : kind_(kind) {}
    

private:
    /// Kind of the statement.
    Kind kind_;
    std::vector<std::string> kindnames = {"block", "while", "expr", "return", "if"};
};

/**
 * Base class for all expressions.
 */
class Expr : public Node
{
public:
    enum class Kind
    {
        REF,
        BINARY,
        UNARY,
        CALL,
        INTEGER,
        BOOL, 
        STRING
    };

public:
    Expr(Kind kind) : kind_(kind) {}

    Kind GetKind() const { return kind_; }

private:
    /// Kind of the expression.
    Kind kind_;
};

/**
 * Integer literal expression.
 */
class IntExpr : public Expr
{
public:
    IntExpr(const uint64_t val)
        : Expr(Kind::INTEGER), val_(val)
    {
    }

    uint64_t GetValue() const { return val_; }

private:
    /// Value of the the integer.
    uint64_t val_;
};

/**
 * Boolean literal expression (true | false).
 */
class BoolExpr : public Expr
{
public:
    BoolExpr(const bool val)
        : Expr(Kind::BOOL), val_(val)
    {
    }

    bool GetValue() const { return val_; }

private:
    /// Value of the the boolean.
    bool val_;
};

/**
 * String literal expression
 */
class StringExpr : public Expr
{
public:
    StringExpr(const std::string_view& string)
        : Expr(Kind::STRING), str_(string)
    {
    }

    const std::string_view& GetString() const { return str_; }

private:
    /// Value of the the boolean.
    std::string_view str_;
};

/**
 * Expression referring to a named value.
 */
class RefExpr : public Expr
{
public:
    RefExpr(const std::string &name)
        : Expr(Kind::REF), name_(name)
    {
    }

    const std::string &GetName() const { return name_; }

private:
    /// Name of the identifier.
    std::string name_;
};

/**
 * Binary expression.
 */
class BinaryExpr : public Expr
{
public:
    /// Enumeration of binary operators.
    enum class Kind
    {
        EQ,
        NEQ,
        MUL,
        DIV, 
        MOD,
        LE,
        GR,
        LEQ,
        GREQ,
        ADD,
        SUB
    };

public:
    BinaryExpr(Kind kind, std::shared_ptr<Expr> lhs, std::shared_ptr<Expr> rhs)
        : Expr(Expr::Kind::BINARY), kind_(kind), lhs_(lhs), rhs_(rhs)
    {
    }

    Kind GetKind() const { return kind_; }

    const Expr &GetLHS() const { return *lhs_; }
    const Expr &GetRHS() const { return *rhs_; }

private:
    /// Operator kind.
    Kind kind_;
    /// Left-hand operand.
    std::shared_ptr<Expr> lhs_;
    /// Right-hand operand.
    std::shared_ptr<Expr> rhs_;
};

/**
 * Unary expression.
 */
class UnaryExpr : public Expr
{
public:
    /// Enumeration of unary operators.
    enum class Kind
    {
        NOT,
        NEG
    };

public:
    UnaryExpr(Kind kind, std::shared_ptr<Expr> operand)
        : Expr(Expr::Kind::UNARY), kind_(kind), operand_(operand)
    {
    }

    Kind GetKind() const { return kind_; }

    const Expr &GetOperand() const { return *operand_; }

private:
    /// Operator kind.
    Kind kind_;
    /// The (only) operand.
    std::shared_ptr<Expr> operand_;

};

/**
 * Call expression.
 */
class CallExpr : public Expr
{
public:
    using ArgList = std::vector<std::shared_ptr<Expr>>;

public:
    CallExpr(
        std::shared_ptr<Expr> callee,
        std::vector<std::shared_ptr<Expr>> &&args)
        : Expr(Kind::CALL), callee_(callee), args_(std::move(args))
    {
    }

    const Expr &GetCallee() const { return *callee_; }

    size_t arg_size() const { return args_.size(); }
    ArgList::const_reverse_iterator arg_rbegin() const { return args_.rbegin(); }
    ArgList::const_reverse_iterator arg_rend() const { return args_.rend(); }

private:
    std::shared_ptr<Expr> callee_;
    ArgList args_;
};

/**
 * Block statement composed of a sequence of statements.
 */
class BlockStmt final : public Stmt
{
public:
    using BlockList = std::vector<std::shared_ptr<Stmt>>;

public:
    BlockStmt(std::vector<std::shared_ptr<Stmt>> &&body)
        : Stmt(Kind::BLOCK), body_(body)
    {
    }

    BlockList::const_iterator begin() const { return body_.begin(); }
    BlockList::const_iterator end() const { return body_.end(); }

private:
    /// Statements in the body of the block.
    BlockList body_;
};

/**
 * Top-level expression statement.
 */
class ExprStmt final : public Stmt
{
public:
    ExprStmt(std::shared_ptr<Expr> expr)
        : Stmt(Kind::EXPR), expr_(expr)
    {
    }

    const Expr &GetExpr() const { return *expr_; }

private:
    /// Top-level expression.
    std::shared_ptr<Expr> expr_;
};

/**
 * Return statement returning an expression.
 */
class ReturnStmt final : public Stmt
{
public:
    ReturnStmt(std::shared_ptr<Expr> expr)
        : Stmt(Kind::RETURN), expr_(expr)
    {
    }

    const Expr &GetExpr() const { return *expr_; }

private:
    /// Expression to be returned.
    std::shared_ptr<Expr> expr_;
};

/**
 * If statement.
 * if (<expr>) <stmt> else <stmt>
 */
class IfStmt final : public Stmt
{
public:
    IfStmt(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> ifTrue, std::shared_ptr<Stmt> ifFalse)
        : Stmt(Kind::IF), tstmt_(ifTrue), fstmt_(ifFalse), cond_(cond)
    {
    }

    const Stmt &GetTrueBranchStmt() const { return *tstmt_; }
    const Stmt &GetFalseBranchStmt() const { return *fstmt_; }
    const Expr &GetCondition() const { return *cond_; }
private:
    /// statement to execute if condition is met
    std::shared_ptr<Stmt> tstmt_;
    /// statement to execute if condition is not met
    std::shared_ptr<Stmt> fstmt_;
    std::shared_ptr<Expr> cond_;
};

/**
 * While statement.
 *
 * while (<cond>) <stmt>
 */
class WhileStmt final : public Stmt
{
public:
    WhileStmt(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> stmt)
        : Stmt(Kind::WHILE), cond_(cond), stmt_(stmt)
    {
    }

    const Expr &GetCond() const { return *cond_; }
    const Stmt &GetStmt() const { return *stmt_; }

private:
    /// Condition for the loop.
    std::shared_ptr<Expr> cond_;
    /// Expression to be executed in the loop body.
    std::shared_ptr<Stmt> stmt_;
};

/**
 * Variable declaration statement.
 *
 * let <ident> : <type> = <expr>;
 */
class VarDeclStmt final : public Stmt
{
public:
    VarDeclStmt(const std::string& name, const std::string& type, std::shared_ptr<Expr> expr)
        : Stmt(Kind::VARDECL), name_(name), type_(type), expr_(expr)
    {
    }

    const std::string& GetName() const { return name_; }
    const std::string& GetType() const { return type_; }
    const Expr& GetExpression() const { return *expr_; }

private:

    std::string name_;
    std::string type_;
    std::shared_ptr<Expr> expr_;
};

/**
 * Base class for internal and external function declarations.
 */
class FuncOrProtoDecl : public Node
{
public:
    using ArgList = std::vector<std::pair<std::string, std::string>>;

public:
    FuncOrProtoDecl(
        const std::string &name,
        std::vector<std::pair<std::string, std::string>> &&args,
        const std::string &type)
        : name_(name), args_(std::move(args)), type_(type)
    {
    }

    virtual ~FuncOrProtoDecl();

    const std::string &GetName() const { return name_; }

    size_t arg_size() const { return args_.size(); }
    ArgList::const_iterator arg_begin() const { return args_.begin(); }
    ArgList::const_iterator arg_end() const { return args_.end(); }


private:
    /// Name of the declaration.
    const std::string name_;
    /// Argument list.
    ArgList args_;
    /// Return type identifier.
    const std::string &type_;
};

/**
 * External function prototype declaration.
 *
 * func proto(a: int): int = "proto"
 */
class ProtoDecl final : public FuncOrProtoDecl
{
public:
    ProtoDecl(
        const std::string &name,
        std::vector<std::pair<std::string, std::string>> &&args,
        const std::string &type,
        const std::string &primitive)
        : FuncOrProtoDecl(name, std::move(args), type), primitive_(primitive)
    {
    }

    const std::string &GetPrimitiveName() const { return primitive_; }

private:
    const std::string primitive_;
};

/**
 * Function declaration.
 *
 * func test(a: int): int = { ... }
 */
class FuncDecl final : public FuncOrProtoDecl
{
public:
    FuncDecl(
        const std::string &name,
        std::vector<std::pair<std::string, std::string>> &&args,
        const std::string &type,
        std::shared_ptr<BlockStmt> body)
        : FuncOrProtoDecl(name, std::move(args), type), body_(body)
    {
    }

    const BlockStmt &GetBody() const { return *body_; }

private:
    std::shared_ptr<BlockStmt> body_;
};

/// Alternative for a toplevel construct.
using TopLevelStmt = std::variant<std::shared_ptr<FuncDecl>, std::shared_ptr<ProtoDecl>, std::shared_ptr<Stmt>>;

/**
 * Main node of the AST, capturing information about the program.
 */
class Module final : public Node
{
public:
    using BlockList = std::vector<TopLevelStmt>;

public:
    Module(
        std::vector<TopLevelStmt> &&body)
        : body_(body)
    {
    }

    BlockList::const_iterator begin() const { return body_.begin(); }
    BlockList::const_iterator end() const { return body_.end(); }
private:
    BlockList body_;
};
