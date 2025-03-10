#include <iostream>
#include <cstdlib>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <string>
#include <unordered_set>
#include "parser.h"

using namespace std;

// ============================================================
// Global Flags and Execution-Related Variables
// ============================================================
bool runTask2 = false; // Task 2: Execution of program statements.
bool runTask3 = false; // Task 3: Check for uninitialized variable warnings.
bool runTask4 = false; // Task 4: Check for useless assignments.
bool runTask5 = false; // Task 5: Output polynomial degrees.

map<string, int> execSymbolTable;  // Maps variable names to memory locations.
vector<int> mem(1000, 0);            // Memory array (size 1000, all initialized to 0).
vector<int> execInputs;              // Stores input values from the INPUTS section.
int nextInputIndex = 0;              // Index of the next input to be read.

// ============================================================
// AST for Expressions
// ============================================================
enum ExprType { CONST_EXPR, VAR_EXPR, ADD_EXPR, SUB_EXPR, MUL_EXPR, POW_EXPR, CALL_EXPR };

// Base class for all expressions.
struct Expr {
    ExprType type;
    virtual int evaluate(const map<string,int>& env) = 0;
    virtual ~Expr() {}
};

// Represents a constant value.
struct ConstExpr : public Expr {
    int value;
    ConstExpr(int v) : value(v) { type = CONST_EXPR; }
    int evaluate(const map<string,int>& env) override { return value; }
};

// Represents a variable.
struct VarExpr : public Expr {
    string name;
    int line_no; // Token's line number for error reporting.
    VarExpr(const string &n, int ln) : name(n), line_no(ln) { type = VAR_EXPR; }
    int evaluate(const map<string,int>& env) override {
        auto it = env.find(name);
        return (it != env.end() ? it->second : 0);
    }
};

// Represents addition.
struct AddExpr : public Expr {
    Expr* left;
    Expr* right;
    AddExpr(Expr* l, Expr* r) : left(l), right(r) { type = ADD_EXPR; }
    int evaluate(const map<string,int>& env) override { return left->evaluate(env) + right->evaluate(env); }
    ~AddExpr() { delete left; delete right; }
};

// Represents subtraction.
struct SubExpr : public Expr {
    Expr* left;
    Expr* right;
    SubExpr(Expr* l, Expr* r) : left(l), right(r) { type = SUB_EXPR; }
    int evaluate(const map<string,int>& env) override { return left->evaluate(env) - right->evaluate(env); }
    ~SubExpr() { delete left; delete right; }
};

// Represents multiplication.
struct MulExpr : public Expr {
    Expr* left;
    Expr* right;
    MulExpr(Expr* l, Expr* r) : left(l), right(r) { type = MUL_EXPR; }
    int evaluate(const map<string,int>& env) override { return left->evaluate(env) * right->evaluate(env); }
    ~MulExpr() { delete left; delete right; }
};

// Represents exponentiation.
struct PowExpr : public Expr {
    Expr* base;
    int exponent;
    PowExpr(Expr* b, int exp) : base(b), exponent(exp) { type = POW_EXPR; }
    int evaluate(const map<string,int>& env) override {
        int res = 1;
        int bval = base->evaluate(env);
        for (int i = 0; i < exponent; i++)
            res *= bval;
        return res;
    }
    ~PowExpr() { delete base; }
};

// Represents a function (polynomial) call.
struct CallExpr : public Expr {
    string polyName;
    vector<Expr*> args; // Argument expressions.
    CallExpr(const string &n, const vector<Expr*> &a) : polyName(n), args(a) { type = CALL_EXPR; }
    int evaluate(const map<string,int>& env) override {
        vector<int> argVals;
        for (Expr* e : args) {
            argVals.push_back(e->evaluate(env));
        }
        return evaluatePolyCall(polyName, argVals);
    }
    ~CallExpr() {
        for (Expr* e : args)
            delete e;
    }
};

// ============================================================
// Global Tables for Polynomial Declarations and ASTs
// ============================================================
map<string, vector<string>> poly_declarations; // Maps polynomial name to its parameter list.
map<string, Expr*> polyExprs;                    // Maps polynomial name to AST for its body.

// ============================================================
// Evaluate a Polynomial Call
// ============================================================
// Looks up the polynomial's declaration and evaluates its body using the provided arguments.
int evaluatePolyCall(const string &polyName, const vector<int> &args) {
    if (poly_declarations.find(polyName) == poly_declarations.end() ||
        polyExprs.find(polyName) == polyExprs.end())
        return 0;
    const vector<string>& params = poly_declarations[polyName];
    map<string,int> env;
    for (size_t i = 0; i < params.size(); i++) {
        int argVal = (i < args.size() ? args[i] : 0);
        env[params[i]] = argVal;
    }
    return polyExprs[polyName]->evaluate(env);
}

// ============================================================
// Execution Statement Classes
// ============================================================
enum StmtType { INPUT_STMT, OUTPUT_STMT, ASSIGN_STMT };

// Base class for execution statements.
struct Statement {
    StmtType type;
    string var;
    virtual void execute() = 0;
    virtual ~Statement() {}
};

// Represents an INPUT statement.
struct InputStmt : public Statement {
    InputStmt(string v) { type = INPUT_STMT; var = v; }
    void execute() override {
        if (execSymbolTable.find(var) == execSymbolTable.end()) {
            int loc = execSymbolTable.size();
            execSymbolTable[var] = loc;
        }
        mem[execSymbolTable[var]] = execInputs[nextInputIndex++];
    }
};

// Represents an OUTPUT statement.
struct OutputStmt : public Statement {
    OutputStmt(string v) { type = OUTPUT_STMT; var = v; }
    void execute() override {
        if (execSymbolTable.find(var) == execSymbolTable.end()) {
            int loc = execSymbolTable.size();
            execSymbolTable[var] = loc;
        }
        cout << mem[execSymbolTable[var]] << endl;
    }
};

// Represents an ASSIGNMENT statement.
struct AssignStmt : public Statement {
    Expr* rhs;  // Right-hand side expression.
    int line_no;
    AssignStmt(string v, int ln, Expr* expr) { 
        type = ASSIGN_STMT; 
        var = v; 
        line_no = ln;
        rhs = expr; 
    }
    void execute() override {
        if (execSymbolTable.find(var) == execSymbolTable.end()) {
            int loc = execSymbolTable.size();
            execSymbolTable[var] = loc;
        }
        int size = execSymbolTable.size();
        map<string,int> env;
        // Build the environment using the current symbol table.
        for (int i = 0; i < size; i++) {
            for (auto &p : execSymbolTable) {
                if (p.second == i) {
                    env[p.first] = mem[i];
                    break;
                }
            }
        }
        mem[execSymbolTable[var]] = rhs->evaluate(env);
    }
    ~AssignStmt() { delete rhs; }
};

vector<Statement*> execStatements; // Global vector to hold execution statements.

// ============================================================
// Parser Implementation (Extended with Execution Methods)
// ============================================================

/*
   The following methods are members of the Parser class.
   They are organized by functionality: syntax utilities, program structure,
   polynomial declaration parsing, expression parsing, execution section parsing,
   input parsing, and static analysis.
*/

// ---------- Syntax Utilities ----------

// Reports a syntax error and terminates the program.
void Parser::syntax_error() {
    cout << "SYNTAX ERROR !!!!!&%!!\n";
    exit(1);
}

// Consumes the next token and checks if it matches the expected type.
Token Parser::expect(TokenType expected_type) {
    Token t = lexer.GetToken();
    if (t.token_type != expected_type)
        syntax_error();
    return t;
}

// ---------- Program Structure Parsing ----------

// Parses the entire program.
void Parser::parse_program() {
    parse_tasks_section();
    parse_poly_section();
    parse_execute_section();
    parse_inputs_section();
}

// Parses the TASKS section and sets corresponding task flags.
void Parser::parse_tasks_section() {
    expect(TASKS);
    parse_num_list();
}

// Parses a list of task numbers.
void Parser::parse_num_list() {
    Token t = expect(NUM);
    int task = atoi(t.lexeme.c_str());
    if (task == 2)
        runTask2 = true;
    if (task == 3)
        runTask3 = true;
    if (task == 4)
        runTask4 = true;
    if (task == 5)
        runTask5 = true;
    Token next = lexer.peek(1);
    if (next.token_type == NUM)
        parse_num_list();
}

// ---------- Polynomial Declaration Parsing ----------

// Parses the POLY section.
void Parser::parse_poly_section() {
    expect(POLY);
    parse_poly_decl_list();
}

// Parses a list of polynomial declarations.
void Parser::parse_poly_decl_list() {
    parse_poly_decl();
    Token t = lexer.peek(1);
    if (t.token_type == ID)
        parse_poly_decl_list();
}

// Parses a single polynomial declaration.
void Parser::parse_poly_decl() {
    PolyHeader header = parse_poly_header();
    expect(EQUAL);
    current_poly_params = header.params;
    in_poly_decl = true;
    // Build the AST for the polynomial body.
    Expr* body = parse_poly_body_expr();
    in_poly_decl = false;
    expect(SEMICOLON);
    if (poly_decl_first_line.find(header.name) != poly_decl_first_line.end()) {
        duplicate_decl_lines.push_back(header.line);
    } else {
        poly_decl_first_line[header.name] = header.line;
        polyExprs[header.name] = body;
    }
}

// Parses the header of a polynomial declaration.
PolyHeader Parser::parse_poly_header() {
    Token t = expect(ID);
    PolyHeader header;
    header.name = t.lexeme;
    header.line = t.line_no;
    Token next = lexer.peek(1);
    if (next.token_type == LPAREN) {
        expect(LPAREN);
        header.params = parse_id_list();
        expect(RPAREN);
    } else {
        header.params.push_back("x");
    }
    poly_declarations[header.name] = header.params;
    return header;
}

// Parses a comma-separated list of identifiers.
vector<string> Parser::parse_id_list() {
    vector<string> ids;
    Token t = expect(ID);
    ids.push_back(t.lexeme);
    Token next = lexer.peek(1);
    while (next.token_type == COMMA) {
        expect(COMMA);
        t = expect(ID);
        ids.push_back(t.lexeme);
        next = lexer.peek(1);
    }
    return ids;
}

// Parses the body of a polynomial expression.
Expr* Parser::parse_poly_body_expr() {
    Expr* left = parse_poly_term();
    while (lexer.peek(1).token_type == PLUS) {
        expect(PLUS);
        Expr* right = parse_poly_term();
        left = new AddExpr(left, right);
    }
    if (lexer.peek(1).token_type == MINUS) {
        expect(MINUS);
        Expr* right = parse_poly_body_expr();  // Right associativity.
        left = new SubExpr(left, right);
    }
    return left;
}

// Parses a term in a polynomial expression.
Expr* Parser::parse_poly_term() {
    Token t = lexer.peek(1);
    Expr* left = nullptr;
    if (t.token_type == NUM) {
        t = expect(NUM);
        left = new ConstExpr(atoi(t.lexeme.c_str()));
        // Handle implicit multiplication.
        while (lexer.peek(1).token_type == ID || lexer.peek(1).token_type == LPAREN) {
            Expr* right = parse_poly_factor();
            left = new MulExpr(left, right);
        }
    } else if (t.token_type == ID || t.token_type == LPAREN) {
        left = parse_poly_factor();
        while (lexer.peek(1).token_type == ID || lexer.peek(1).token_type == LPAREN) {
            Expr* right = parse_poly_factor();
            left = new MulExpr(left, right);
        }
    } else {
        syntax_error();
    }
    return left;
}

// Parses a factor in a polynomial expression.
Expr* Parser::parse_poly_factor() {
    Expr* expr = nullptr;
    bool wasParen = false;
    Token t = lexer.peek(1);
    if (t.token_type == NUM) {
        t = expect(NUM);
        expr = new ConstExpr(atoi(t.lexeme.c_str()));
        wasParen = false;
    } else if (t.token_type == ID) {
        Token idToken = t;
        t = expect(ID);
        // Check if the identifier is in the current parameter list.
        if (find(current_poly_params.begin(), current_poly_params.end(), idToken.lexeme) == current_poly_params.end())
            invalid_monomial_lines.push_back(idToken.line_no);
        expr = new VarExpr(idToken.lexeme, idToken.line_no);
        wasParen = false;
    } else if (t.token_type == LPAREN) {
        expect(LPAREN);
        expr = parse_poly_body_expr();
        expect(RPAREN);
        wasParen = true;
    } else {
        syntax_error();
    }
    t = lexer.peek(1);
    if (t.token_type == POWER) {
        expect(POWER);
        Token num = expect(NUM);
        int exp = atoi(num.lexeme.c_str());
        expr = new PowExpr(expr, exp);
        wasParen = false;
    }
    // Disallow a parenthesized expression immediately followed by a number in poly declarations.
    else if (in_poly_decl && wasParen && t.token_type == NUM)
        syntax_error();
    return expr;
}

// ---------- Expression Parsing Functions ----------

// Parses an expression with addition and subtraction.
Expr* Parser::parse_expr() {
    Expr* left = parse_term_expr();
    while (lexer.peek(1).token_type == PLUS) {
        expect(PLUS);
        Expr* right = parse_term_expr();
        left = new AddExpr(left, right);
    }
    if (lexer.peek(1).token_type == MINUS) {
        expect(MINUS);
        Expr* right = parse_expr();  // Right associativity.
        left = new SubExpr(left, right);
    }
    return left;
}

// Parses a term in an expression, handling implicit multiplication.
Expr* Parser::parse_term_expr() {
    Expr* left = parse_factor();
    if (in_poly_decl) {
        while (lexer.peek(1).token_type == ID || lexer.peek(1).token_type == LPAREN) {
            Expr* right = parse_factor();
            left = new MulExpr(left, right);
        }
    } else {
        Token t = lexer.peek(1);
        while (t.token_type == ID || t.token_type == LPAREN) {
            if (left->type == CALL_EXPR)
                break;
            Expr* right = parse_factor();
            left = new MulExpr(left, right);
            t = lexer.peek(1);
        }
    }
    return left;
}

// Parses a factor in an expression.
Expr* Parser::parse_factor() {
    Expr* expr = nullptr;
    bool wasParen = false;
    Token t = lexer.peek(1);
    if (t.token_type == NUM) {
        t = expect(NUM);
        expr = new ConstExpr(atoi(t.lexeme.c_str()));
        wasParen = false;
    } else if (t.token_type == ID) {
        Token idToken = t;
        t = expect(ID);
        if (in_poly_decl) {
            if (find(current_poly_params.begin(), current_poly_params.end(), idToken.lexeme) == current_poly_params.end())
                invalid_monomial_lines.push_back(idToken.line_no);
            expr = new VarExpr(idToken.lexeme, idToken.line_no);
        } else {
            if (lexer.peek(1).token_type == LPAREN)
                expr = parse_call_expr_from_token(idToken);
            else
                expr = new VarExpr(idToken.lexeme, idToken.line_no);
        }
        wasParen = false;
    } else if (t.token_type == LPAREN) {
        expect(LPAREN);
        expr = parse_expr();
        expect(RPAREN);
        wasParen = true;
    } else {
        syntax_error();
    }
    t = lexer.peek(1);
    if (t.token_type == POWER) {
        expect(POWER);
        Token num = expect(NUM);
        int exp = atoi(num.lexeme.c_str());
        expr = new PowExpr(expr, exp);
        wasParen = false;
    }
    else if (in_poly_decl && wasParen && t.token_type == NUM)
        syntax_error();
    return expr;
}

// Parses an argument expression used in function calls.
Expr* Parser::parse_argument_expr() {
    Token t = lexer.peek(1);
    if (t.token_type == ID) {
        Token t2 = lexer.peek(2);
        if (t2.token_type == LPAREN)
            return parse_call_expr_from_token(expect(ID));
        else {
            t = expect(ID);
            return new VarExpr(t.lexeme, t.line_no);
        }
    } else if (t.token_type == NUM) {
        t = expect(NUM);
        return new ConstExpr(atoi(t.lexeme.c_str()));
    } else {
        syntax_error();
        return nullptr;
    }
}

// Helper: Parses a call expression using an already-consumed ID token.
Expr* Parser::parse_call_expr_from_token(Token idToken) {
    string polyName = idToken.lexeme;
    int eval_line = idToken.line_no;
    expect(LPAREN);
    vector<Expr*> args;
    args.push_back(parse_argument_expr());
    while (lexer.peek(1).token_type == COMMA) {
        expect(COMMA);
        args.push_back(parse_argument_expr());
    }
    expect(RPAREN);
    if (poly_declarations.find(polyName) != poly_declarations.end()) {
        int expected = poly_declarations[polyName].size();
        if ((int)args.size() != expected)
            semantic_errors.push_back(string("Semantic Error Code 4: ") + to_string(eval_line));
    } else {
        semantic_errors.push_back(string("Semantic Error Code 3: ") + to_string(eval_line));
    }
    return new CallExpr(polyName, args);
}

// ---------- Execution Section Parsing ----------

// Parses the EXECUTE section.
void Parser::parse_execute_section() {
    expect(EXECUTE);
    parse_statement_list_exec();
}

// Parses a list of execution statements.
void Parser::parse_statement_list_exec() {
    Statement* stmt = parse_statement_exec();
    execStatements.push_back(stmt);
    Token t = lexer.peek(1);
    if (t.token_type == INPUT || t.token_type == OUTPUT || t.token_type == ID)
        parse_statement_list_exec();
}

// Parses a single execution statement.
Statement* Parser::parse_statement_exec() {
    Token t = lexer.peek(1);
    if (t.token_type == INPUT)
        return parse_input_statement_exec();
    else if (t.token_type == OUTPUT)
        return parse_output_statement_exec();
    else if (t.token_type == ID)
        return parse_assign_statement_exec();
    else
        syntax_error();
    return nullptr;
}

// Parses an INPUT statement.
Statement* Parser::parse_input_statement_exec() {
    expect(INPUT);
    Token t = expect(ID);
    expect(SEMICOLON);
    return new InputStmt(t.lexeme);
}

// Parses an OUTPUT statement.
Statement* Parser::parse_output_statement_exec() {
    expect(OUTPUT);
    Token t = expect(ID);
    expect(SEMICOLON);
    return new OutputStmt(t.lexeme);
}

// Parses an ASSIGNMENT statement.
Statement* Parser::parse_assign_statement_exec() {
    Token t = expect(ID);
    string varName = t.lexeme;
    int assignLine = t.line_no;  // Record the line number.
    expect(EQUAL);
    Expr* callExpr = parse_poly_eval_expr();
    expect(SEMICOLON);
    return new AssignStmt(varName, assignLine, callExpr);
}

// Parses a polynomial evaluation expression used in assignments.
Expr* Parser::parse_poly_eval_expr() {
    Token t = expect(ID);
    string polyName = t.lexeme;
    used_polynomials.insert(polyName);
    int eval_line = t.line_no;
    expect(LPAREN);
    vector<Expr*> args;
    args.push_back(parse_argument_expr());
    while (lexer.peek(1).token_type == COMMA) {
        expect(COMMA);
        args.push_back(parse_argument_expr());
    }
    expect(RPAREN);
    if (poly_declarations.find(polyName) != poly_declarations.end()) {
        int expected = poly_declarations[polyName].size();
        if ((int)args.size() != expected)
            semantic_errors.push_back(string("Semantic Error Code 4: ") + to_string(eval_line));
    } else {
        semantic_errors.push_back(string("Semantic Error Code 3: ") + to_string(eval_line));
    }
    return new CallExpr(polyName, args);
}

// Parses a call expression (alternative interface).
Statement* Parser::parse_call_expr() {
    Token t = expect(ID);
    string polyName = t.lexeme;
    int eval_line = t.line_no;
    expect(LPAREN);
    vector<Expr*> args;
    args.push_back(parse_argument_expr());
    while (lexer.peek(1).token_type == COMMA) {
        expect(COMMA);
        args.push_back(parse_argument_expr());
    }
    expect(RPAREN);
    if (poly_declarations.find(polyName) != poly_declarations.end()) {
        int expected = poly_declarations[polyName].size();
        if ((int)args.size() != expected)
            semantic_errors.push_back(string("Semantic Error Code 4: ") + to_string(eval_line));
    } else {
        semantic_errors.push_back(string("Semantic Error Code 3: ") + to_string(eval_line));
    }
    return new CallExpr(polyName, args);
}

// Parses a polynomial evaluation used in an argument list.
string Parser::parse_poly_evaluation() {
    Token t = expect(ID);
    string poly_name = t.lexeme;
    int eval_line = t.line_no;
    used_polynomials.insert(poly_name);
    expect(LPAREN);
    parse_argument_list(poly_name, eval_line);
    expect(RPAREN);
    return poly_name;
}

// Parses the argument list for a polynomial evaluation.
void Parser::parse_argument_list(string poly_name, int eval_line) {
    bool undeclared = false;
    if (poly_declarations.find(poly_name) == poly_declarations.end()) {
        semantic_errors.push_back("Semantic Error Code 3: " + to_string(eval_line));
        undeclared = true;
    }
    int expected_params = undeclared ? 0 : (int)poly_declarations[poly_name].size();
    int actual_params = 1;
    parse_argument();
    Token t = lexer.peek(1);
    while (t.token_type == COMMA) {
        expect(COMMA);
        parse_argument();
        actual_params++;
        t = lexer.peek(1);
    }
    if (!undeclared && actual_params != expected_params)
        semantic_errors.push_back("Semantic Error Code 4: " + to_string(eval_line));
}

// Parses a single argument.
void Parser::parse_argument() {
    Token t = lexer.peek(1);
    if (t.token_type == ID) {
        Token t2 = lexer.peek(2);
        if (t2.token_type == LPAREN)
            parse_poly_evaluation();
        else
            expect(ID);
    } else if (t.token_type == NUM) {
        expect(NUM);
    } else {
        syntax_error();
    }
}

// ---------- Input Section Parsing ----------

// Parses the INPUTS section.
void Parser::parse_inputs_section() {
    expect(INPUTS);
    if (lexer.peek(1).token_type != NUM)
        syntax_error();
    while (lexer.peek(1).token_type == NUM) {
        Token t = expect(NUM);
        execInputs.push_back(atoi(t.lexeme.c_str()));
    }
}

// ---------- Execution Phase ----------
// Executes all parsed execution statements in order.
void Parser::execute_program() {
    for (auto stmt : execStatements) {
        stmt->execute();
    }
}

// ---------- Semantic Error Checking ----------
// Checks for semantic errors in the program and terminates if found.
void Parser::check_semantic_errors() {
    if (!duplicate_decl_lines.empty()) {
        sort(duplicate_decl_lines.begin(), duplicate_decl_lines.end());
        cout << "Semantic Error Code 1:";
        for (int line : duplicate_decl_lines)
            cout << " " << line;
        cout << endl;
        exit(0);
    }
    if (!invalid_monomial_lines.empty()) {
        sort(invalid_monomial_lines.begin(), invalid_monomial_lines.end());
        cout << "Semantic Error Code 2:";
        for (int line : invalid_monomial_lines)
            cout << " " << line;
        cout << endl;
        exit(0);
    }
    if (!semantic_errors.empty()) {
        vector<int> lines;
        if (semantic_errors[0].find("Semantic Error Code 3:") == 0) {
            for (const string &err : semantic_errors) {
                size_t pos = err.find(":");
                if (pos != string::npos) {
                    int n = stoi(err.substr(pos + 1));
                    lines.push_back(n);
                }
            }
            sort(lines.begin(), lines.end());
            string combined = "Semantic Error Code 3:";
            for (int n : lines)
                combined += " " + to_string(n);
            cout << combined << endl;
        } else if (semantic_errors[0].find("Semantic Error Code 4:") == 0) {
            for (const string &err : semantic_errors) {
                size_t pos = err.find(":");
                if (pos != string::npos) {
                    int n = stoi(err.substr(pos + 1));
                    lines.push_back(n);
                }
            }
            sort(lines.begin(), lines.end());
            string combined = "Semantic Error Code 4:";
            for (int n : lines)
                combined += " " + to_string(n);
            cout << combined << endl;
        } else {
            for (const string &err : semantic_errors)
                cout << err << endl;
        }
        exit(0);
    }
}

// ---------- Static Analysis Functions ----------

// Recursively traverses an expression AST and records warnings for uninitialized variables.
void check_expr_for_uninit(Expr* expr, const unordered_set<string>& initVars, vector<int>& warnings) {
    if (!expr) return;
    switch(expr->type) {
        case CONST_EXPR:
            break;
        case VAR_EXPR: {
            VarExpr* ve = static_cast<VarExpr*>(expr);
            if (initVars.find(ve->name) == initVars.end())
                warnings.push_back(ve->line_no);
            break;
        }
        case ADD_EXPR: {
            AddExpr* ae = static_cast<AddExpr*>(expr);
            check_expr_for_uninit(ae->left, initVars, warnings);
            check_expr_for_uninit(ae->right, initVars, warnings);
            break;
        }
        case SUB_EXPR: {
            SubExpr* se = static_cast<SubExpr*>(expr);
            check_expr_for_uninit(se->left, initVars, warnings);
            check_expr_for_uninit(se->right, initVars, warnings);
            break;
        }
        case MUL_EXPR: {
            MulExpr* me = static_cast<MulExpr*>(expr);
            check_expr_for_uninit(me->left, initVars, warnings);
            check_expr_for_uninit(me->right, initVars, warnings);
            break;
        }
        case POW_EXPR: {
            PowExpr* pe = static_cast<PowExpr*>(expr);
            check_expr_for_uninit(pe->base, initVars, warnings);
            break;
        }
        case CALL_EXPR: {
            CallExpr* ce = static_cast<CallExpr*>(expr);
            for (Expr* arg : ce->args)
                check_expr_for_uninit(arg, initVars, warnings);
            break;
        }
    }
}

// Checks if an expression uses a given variable.
bool exprUsesVar(Expr* expr, const string &var) {
    if (!expr) return false;
    switch(expr->type) {
        case CONST_EXPR:
            return false;
        case VAR_EXPR: {
            VarExpr* ve = static_cast<VarExpr*>(expr);
            return (ve->name == var);
        }
        case ADD_EXPR: {
            AddExpr* ae = static_cast<AddExpr*>(expr);
            return exprUsesVar(ae->left, var) || exprUsesVar(ae->right, var);
        }
        case SUB_EXPR: {
            SubExpr* se = static_cast<SubExpr*>(expr);
            return exprUsesVar(se->left, var) || exprUsesVar(se->right, var);
        }
        case MUL_EXPR: {
            MulExpr* me = static_cast<MulExpr*>(expr);
            return exprUsesVar(me->left, var) || exprUsesVar(me->right, var);
        }
        case POW_EXPR: {
            PowExpr* pe = static_cast<PowExpr*>(expr);
            return exprUsesVar(pe->base, var);
        }
        case CALL_EXPR: {
            CallExpr* ce = static_cast<CallExpr*>(expr);
            for (Expr* arg : ce->args)
                if (exprUsesVar(arg, var))
                    return true;
            return false;
        }
    }
    return false;
}

// Checks for assignments whose values are never used.
void checkUselessAssignments() {
    vector<int> uselessLines;
    for (int i = 0; i < execStatements.size(); i++) {
        if (execStatements[i]->type != ASSIGN_STMT) continue;
        AssignStmt* currAssign = static_cast<AssignStmt*>(execStatements[i]);
        string var = currAssign->var;
        bool used = false;
        for (int j = i + 1; j < execStatements.size(); j++) {
            Statement* s = execStatements[j];
            // Check if a redefinition or usage of 'var' occurs.
            if (s->type == ASSIGN_STMT && s->var == var) {
                AssignStmt* as = static_cast<AssignStmt*>(s);
                if (exprUsesVar(as->rhs, var))
                    used = true;
                break;
            }
            if (s->type == INPUT_STMT && s->var == var)
                break;
            if (s->type == OUTPUT_STMT && s->var == var) {
                used = true;
                break;
            }
            if (s->type == ASSIGN_STMT && s->var != var) {
                AssignStmt* as = static_cast<AssignStmt*>(s);
                if (exprUsesVar(as->rhs, var)) {
                    used = true;
                    break;
                }
            }
        }
        if (!used)
            uselessLines.push_back(currAssign->line_no);
    }
    if (!uselessLines.empty()) {
        sort(uselessLines.begin(), uselessLines.end());
        cout << "Warning Code 2:";
        for (int ln : uselessLines)
            cout << " " << ln;
        cout << endl;
    }
}

// Performs static analysis on the EXECUTE section for uninitialized variable usage.
void checkUninitializedWarnings() {
    unordered_set<string> initializedVars;
    vector<int> warningLines;
    for (Statement* stmt : execStatements) {
        if (stmt->type == INPUT_STMT) {
            initializedVars.insert(stmt->var);
        } else if (stmt->type == ASSIGN_STMT) {
            AssignStmt* assignStmt = static_cast<AssignStmt*>(stmt);
            check_expr_for_uninit(assignStmt->rhs, initializedVars, warningLines);
            initializedVars.insert(stmt->var);
        }
    }
    if (!warningLines.empty()) {
        sort(warningLines.begin(), warningLines.end());
        cout << "Warning Code 1:";
        for (int ln : warningLines)
            cout << " " << ln;
        cout << endl;
    }
}

// Computes the degree of a polynomial represented by an expression AST.
int computeDegree(Expr* expr) {
    if (!expr) return 0;
    switch(expr->type) {
        case CONST_EXPR:
            return 0;
        case VAR_EXPR:
            return 1;
        case ADD_EXPR: {
            AddExpr* ae = static_cast<AddExpr*>(expr);
            int d1 = computeDegree(ae->left);
            int d2 = computeDegree(ae->right);
            return max(d1, d2);
        }
        case SUB_EXPR: {
            SubExpr* se = static_cast<SubExpr*>(expr);
            int d1 = computeDegree(se->left);
            int d2 = computeDegree(se->right);
            return max(d1, d2);
        }
        case MUL_EXPR: {
            MulExpr* me = static_cast<MulExpr*>(expr);
            return computeDegree(me->left) + computeDegree(me->right);
        }
        case POW_EXPR: {
            PowExpr* pe = static_cast<PowExpr*>(expr);
            int baseDegree = computeDegree(pe->base);
            return baseDegree * pe->exponent;
        }
        case CALL_EXPR:
            return 0;
    }
    return 0;
}

// Outputs the degrees of all declared polynomials in order.
void outputPolynomialDegrees() {
    vector<pair<int, string>> polyOrder;
    for (auto &entry : poly_decl_first_line) {
        polyOrder.push_back({entry.second, entry.first});
    }
    sort(polyOrder.begin(), polyOrder.end());
    for (auto &p : polyOrder) {
        string polyName = p.second;
        int degree = computeDegree(polyExprs[polyName]);
        cout << polyName << ": " << degree << endl;
    }
}

// ============================================================
// Parser Finalization and Main Execution
// ============================================================

// Consumes all input, performs final semantic checks, and terminates if errors exist.
void Parser::ConsumeAllInput() {
    parse_program();
    Token t = lexer.peek(1);
    if (t.token_type != END_OF_FILE)
        syntax_error();
    expect(END_OF_FILE);
    check_semantic_errors();
}

// Main function: initializes the parser, processes input, executes tasks, and performs static analysis.
int main() {
    Parser parser;
    parser.ConsumeAllInput();
    if (runTask2)
        parser.execute_program();
    if (runTask3)
        checkUninitializedWarnings();
    if (runTask4)
        checkUselessAssignments();
    if (runTask5)
        outputPolynomialDegrees();
    return 0;
}
