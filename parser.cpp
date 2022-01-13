// This file is part of the IMP project.

#include <sstream>

#include "parser.h"
#include "lexer.h"
#include "ast.h"

#define LOG 1

// -----------------------------------------------------------------------------
static std::string FormatMessage(const Location &loc, const std::string &msg)
{
    std::ostringstream os;
    os << "[" << loc.Name << ":" << loc.Line << ":" << loc.Column << "] " << msg;
    return os.str();
}

// -----------------------------------------------------------------------------
ParserError::ParserError(const Location &loc, const std::string &msg)
    : std::runtime_error(FormatMessage(loc, msg))
{
}

// -----------------------------------------------------------------------------
Parser::Parser(Lexer &lexer)
    : lexer_(lexer)
{
}

// -----------------------------------------------------------------------------
std::shared_ptr<Module> Parser::ParseModule()
{
    std::vector<TopLevelStmt> body;
    while (auto tk = Current())
    {
        if (tk.Is(Token::Kind::FUNC))
        {
            // Parse a function prototype or declaration.
            std::string name(Expect(Token::Kind::IDENT).GetIdent());
            Expect(Token::Kind::LPAREN);

            std::vector<std::pair<std::string, std::string>> args;
            while (!lexer_.Next().Is(Token::Kind::RPAREN))
            {
                std::string arg(Current().GetIdent());
                Expect(Token::Kind::COLON);
                std::string type(Expect(Token::Kind::IDENT).GetIdent());
                args.emplace_back(arg, type);

                if (!lexer_.Next().Is(Token::Kind::COMMA))
                {
                    break;
                }
            }
            Check(Token::Kind::RPAREN);

            Expect(Token::Kind::COLON);
            std::string type(Expect(Token::Kind::IDENT).GetIdent());

            if (lexer_.Next().Is(Token::Kind::EQ))
            {
                std::string primitive(Expect(Token::Kind::STRING).GetString());
                lexer_.Next();
                auto decl = std::make_shared<ProtoDecl>(
                    name,
                    std::move(args),
                    type,
                    primitive);
                body.push_back(decl);
            }
            else
            {
                auto block = ParseBlockStmt();
                body.push_back(std::make_shared<FuncDecl>(
                    name,
                    std::move(args),
                    type,
                    block));
            }
        }
        else
        {
            // Parse a top-level statement.
            body.push_back(ParseStmt());
        }
    }
    return std::make_unique<Module>(std::move(body));
}

// -----------------------------------------------------------------------------
std::shared_ptr<Stmt> Parser::ParseStmt()
{
    auto tk = Current();
    switch (tk.GetKind())
    {
    case Token::Kind::RETURN:
        return ParseReturnStmt();
    case Token::Kind::WHILE:
        return ParseWhileStmt();
    case Token::Kind::IF:
        return ParseIfStmt();
    case Token::Kind::LET:
        return ParseVarDeclStmt();
    case Token::Kind::LBRACE:
        return ParseBlockStmt();
    default:
        return std::make_shared<ExprStmt>(ParseExpr());
    }
}

// -----------------------------------------------------------------------------
std::shared_ptr<BlockStmt> Parser::ParseBlockStmt()
{
    Check(Token::Kind::LBRACE);

    std::vector<std::shared_ptr<Stmt>> body;
    while (!lexer_.Next().Is(Token::Kind::RBRACE))
    {
        body.push_back(ParseStmt());
        if (!Current().Is(Token::Kind::SEMI))
        {
            break;
        }
    }
    Check(Token::Kind::RBRACE);
    lexer_.Next();
    return std::make_shared<BlockStmt>(std::move(body));
}

// -----------------------------------------------------------------------------
std::shared_ptr<ReturnStmt> Parser::ParseReturnStmt()
{
    Check(Token::Kind::RETURN);
    lexer_.Next();
    auto expr = ParseExpr();
    return std::make_shared<ReturnStmt>(expr);
}

// -----------------------------------------------------------------------------
std::shared_ptr<WhileStmt> Parser::ParseWhileStmt()
{
    Check(Token::Kind::WHILE);
    Expect(Token::Kind::LPAREN);
    lexer_.Next();
    auto cond = ParseExpr();
    Check(Token::Kind::RPAREN);
    lexer_.Next();
    auto stmt = ParseStmt();
    return std::make_shared<WhileStmt>(cond, stmt);
}

// -----------------------------------------------------------------------------
std::shared_ptr<IfStmt> Parser::ParseIfStmt()
{
    Check(Token::Kind::IF);
    Expect(Token::Kind::LPAREN);
    lexer_.Next();
    auto cond = ParseExpr();
    Check(Token::Kind::RPAREN);
    lexer_.Next();
    auto tstmt = ParseStmt();

    //if no else branch, return stmt with null as falsy statement for now
    if (!Current().Is(Token::Kind::ELSE))
        return std::make_shared<IfStmt>(cond, tstmt, nullptr);
    //otherwise parse false branch
    lexer_.Next();
    auto fstmt = ParseStmt();
    return std::make_shared<IfStmt>(cond, tstmt, fstmt);
}

std::shared_ptr<VarDeclStmt> Parser::ParseVarDeclStmt() {
    Check(Token::Kind::LET);
    Expect(Token::Kind::IDENT);
    std::string name(Current().GetIdent());
    Expect(Token::Kind::COLON);
    Expect(Token::Kind::IDENT);
    std::string type(Current().GetIdent());
    Expect(Token::Kind::EQ);
    lexer_.Next();
    auto expr = ParseExpr();
    Check(Token::Kind::SEMI);
    return std::make_shared<VarDeclStmt>(name, type, expr);
}

// -----------------------------------------------------------------------------
std::shared_ptr<Expr> Parser::ParseTermExpr()
{
    auto tk = Current();
    switch (tk.GetKind())
    {
    case Token::Kind::INT:
    {
        uint64_t val(tk.GetInteger());
        lexer_.Next();
        return std::static_pointer_cast<Expr>(
            std::make_shared<IntExpr>(val));
    }
    case Token::Kind::TRUE:
    {
        lexer_.Next();
        return std::static_pointer_cast<Expr>(
            std::make_shared<BoolExpr>(true));
    }
    case Token::Kind::FALSE:
    {
        lexer_.Next();
        return std::static_pointer_cast<Expr>(
            std::make_shared<BoolExpr>(false));
    }
    case Token::Kind::STRING:
    {
        lexer_.Next();
        return std::static_pointer_cast<Expr>(
            std::make_shared<StringExpr>(tk.GetString()));
    }
    case Token::Kind::IDENT:
    {
        std::string ident(tk.GetIdent());
        lexer_.Next();
        return std::static_pointer_cast<Expr>(
            std::make_shared<RefExpr>(ident));
    }
    default:
    {
        //match ( expr )
        if (Current().Is(Token::Kind::LPAREN)) {
            lexer_.Next();
            auto expr = ParseExpr();
            Check(Token::Kind::RPAREN);
            lexer_.Next();
            return expr;
        }

        //otherwise syntax error
        std::ostringstream os;
        os << "unexpected " << tk << ", expecting term";
        Error(tk.GetLocation(), os.str());
    }
    }
}

// -----------------------------------------------------------------------------
std::shared_ptr<Expr> Parser::ParseCallExpr()
{
    
    // function calls like fn()() not allowed, not a functional language (yet)
    std::shared_ptr<Expr> expr = ParseTermExpr();
    if (Current().Is(Token::Kind::LPAREN)) {
        return ParseArgumentList(expr);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::ParseArgumentList(std::shared_ptr<Expr> callee)
{
    std::vector<std::shared_ptr<Expr>> args;
    while (!lexer_.Next().Is(Token::Kind::RPAREN))
        {
            args.push_back(ParseExpr());
            if (!Current().Is(Token::Kind::COMMA))
            {
                break;
            }
        }
    Check(Token::Kind::RPAREN);
    lexer_.Next();
    return std::make_shared<CallExpr>(callee, std::move(args));
}

// -----------------------------------------------------------------------------
std::shared_ptr<Expr> Parser::ParseAddSubExpr()
{
    auto lhs = ParseMulDivExpr();
    auto kind = Current().GetKind();
    while (kind == Token::Kind::PLUS || kind == Token::Kind::MINUS)
    {
        lexer_.Next();
        auto rhs = ParseMulDivExpr();
        lhs = (kind == Token::Kind::PLUS) ? std::make_shared<BinaryExpr>(BinaryExpr::Kind::ADD, lhs, rhs)
                                           : std::make_shared<BinaryExpr>(BinaryExpr::Kind::SUB, lhs, rhs);
        kind = Current().GetKind();
    }
    return lhs;
}

// -----------------------------------------------------------------------------
std::shared_ptr<Expr> Parser::ParseEqualityExpr() {
    auto lhs = ParseComparisonExpr();
    auto kind = Current().GetKind();
    while (kind == Token::Kind::EQEQ || kind == Token::Kind::NEQ) {
        lexer_.Next();
        auto rhs = ParseComparisonExpr();
        lhs = (kind == Token::Kind::EQEQ) ? std::make_shared<BinaryExpr>(BinaryExpr::Kind::EQ, lhs, rhs)
                                          : std::make_shared<BinaryExpr>(BinaryExpr::Kind::NEQ, lhs, rhs);
        kind = Current().GetKind();
    }
    return lhs;
}

std::shared_ptr<Expr> Parser::ParseComparisonExpr() {
    auto lhs = ParseAddSubExpr();
    auto kind = Current().GetKind();
    while (kind == Token::Kind::GR || kind == Token::Kind::GREQ 
        || kind == Token::Kind::LE || kind == Token::Kind::LEQ) {
        lexer_.Next();
        auto rhs = ParseAddSubExpr();
        switch(kind) {
            case Token::Kind::GR :
                lhs =  std::make_shared<BinaryExpr>(BinaryExpr::Kind::GR, lhs, rhs);
                break;
            case Token::Kind::GREQ :
                lhs = std::make_shared<BinaryExpr>(BinaryExpr::Kind::GREQ, lhs, rhs);
                break;
            case Token::Kind::LE :
                lhs = std::make_shared<BinaryExpr>(BinaryExpr::Kind::LE, lhs, rhs);
                break;
            case Token::Kind::LEQ :
                lhs =  std::make_shared<BinaryExpr>(BinaryExpr::Kind::LEQ, lhs, rhs);
                break;
            default:
                assert(false && "Unreachable code");
        }
        kind = Current().GetKind();
    }
    return lhs;
}

std::shared_ptr<Expr> Parser::ParseMulDivExpr() {
    auto lhs = ParseUnaryExpr();
    auto kind = Current().GetKind();
    while (kind == Token::Kind::STAR || kind == Token::Kind::SLASH || kind == Token::Kind::MOD) {
        lexer_.Next();
        auto rhs = ParseUnaryExpr();
        switch(kind) {
            case Token::Kind::STAR :
                lhs = std::make_shared<BinaryExpr>(BinaryExpr::Kind::MUL, lhs, rhs);
                break;
            case Token::Kind::SLASH :
                lhs =  std::make_shared<BinaryExpr>(BinaryExpr::Kind::DIV, lhs, rhs);
                break;
            case Token::Kind::MOD :
                lhs =  std::make_shared<BinaryExpr>(BinaryExpr::Kind::MOD, lhs, rhs);
                break;
            default:
                assert(false && "Unreachable code");
        }
        kind = Current().GetKind();
    }
    return lhs;
}

std::shared_ptr<Expr> Parser::ParseUnaryExpr() {
    switch (Current().GetKind()) {
        case Token::Kind::BANG : {
            auto operand = ParseUnaryExpr();

            return std::make_shared<UnaryExpr>(UnaryExpr::Kind::NOT, operand);
        }
        case Token::Kind::MINUS : {
            auto operand = ParseUnaryExpr();
            return std::make_shared<UnaryExpr>(UnaryExpr::Kind::NEG, operand);
        }
        default:
            return ParseCallExpr();
    }
}

// -----------------------------------------------------------------------------
const Token &Parser::Expect(Token::Kind kind)
{
    lexer_.Next();
    return Check(kind);
}

// -----------------------------------------------------------------------------
const Token &Parser::Check(Token::Kind kind)
{
    const auto &tk = Current();
    if (kind != tk.GetKind())
    {
        std::ostringstream os;
        os << "unexpected " << tk << ", expecting " << kind;
        Error(tk.GetLocation(), os.str());
    }
    return tk;
}

// -----------------------------------------------------------------------------
void Parser::Error(const Location &loc, const std::string &msg)
{
    throw ParserError(loc, msg);
}
