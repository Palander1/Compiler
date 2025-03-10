#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include <vector>
#include <unordered_set>
#include "lexer.h"

using namespace std;

// Structure representing a polynomial header.
struct PolyHeader {
    string name;            // Name of the polynomial.
    int line;               // Line number of declaration.
    vector<string> params;  // List of parameter names.
};

class Expr;
class Statement;

class Parser {
  public:
    // Consumes all input, performs final semantic checks, and terminates if errors exist.
    void ConsumeAllInput();

    // Executes the parsed program's execution section.
    void execute_program();

  private:
    LexicalAnalyzer lexer;  // Lexical analyzer instance.

    // ---------- Syntax Utilities ----------
    // Reports a syntax error and terminates the program.
    void syntax_error();
    // Consumes the next token and checks if it matches the expected type.
    Token expect(TokenType expected_type);

    // ---------- Program Structure Parsing (Task 1) ----------
    // Parses the entire program.
    void parse_program();
    // Parses the TASKS section and sets task flags.
    void parse_tasks_section();
    // Parses a list of task numbers.
    void parse_num_list();
    // Parses the POLY section.
    void parse_poly_section();
    // Parses a list of polynomial declarations.
    void parse_poly_decl_list();
    // Parses a single polynomial declaration.
    void parse_poly_decl();
    // Parses the header of a polynomial declaration.
    PolyHeader parse_poly_header();
    // Parses a comma-separated list of identifiers.
    vector<string> parse_id_list();
    // Parses the body of a polynomial declaration.
    void parse_poly_body();
    // Parses a list of terms in a polynomial.
    void parse_term_list();
    // Parses a single term.
    void parse_term();
    // Parses a list of monomials.
    void parse_monomial_list();
    // Parses a single monomial.
    void parse_monomial();
    // Parses a primary expression.
    void parse_primary();

    // ---------- Expression Parsing Functions (AST Construction) ----------
    // Parses an expression.
    Expr* parse_expr();
    // Parses a term in an expression.
    Expr* parse_term_expr();
    // Parses a factor in an expression.
    Expr* parse_factor();
    // Parses a call expression given an already-consumed ID token.
    Expr* parse_call_expr_from_token(Token idToken);
    // Parses an argument expression.
    Expr* parse_argument_expr();
    // Parses a polynomial evaluation expression.
    Expr* parse_poly_eval_expr();
    // Parses a factor in a polynomial expression.
    Expr* parse_poly_factor();
    // Parses a term in a polynomial expression.
    Expr* parse_poly_term();
    // Parses the body of a polynomial expression.
    Expr* parse_poly_body_expr();

    // ---------- Execution Section Parsing (Task 2) ----------
    // Parses the EXECUTE section.
    void parse_execute_section();
    // Parses a list of execution statements.
    void parse_statement_list_exec();
    // Parses a single execution statement.
    Statement* parse_statement_exec();
    // Parses an INPUT statement.
    Statement* parse_input_statement_exec();
    // Parses an OUTPUT statement.
    Statement* parse_output_statement_exec();
    // Parses an assignment statement.
    Statement* parse_assign_statement_exec();
    // Parses a call expression used in execution.
    Expr* parse_call_expr();
    // Parses a polynomial evaluation used in execution.
    string parse_poly_evaluation();
    // Parses the argument list for a polynomial evaluation.
    void parse_argument_list(string poly_name, int eval_line);
    // Parses a single argument.
    void parse_argument();
    // Parses the INPUTS section.
    void parse_inputs_section();

    // ---------- Semantic and Static Analysis ----------
    // Checks for semantic errors in the program.
    void check_semantic_errors();
    // Checks for uninitialized variable usage warnings.
    void checkUninitializedWarnings();
    // Recursively traverses an expression AST to record warnings for uninitialized variables.
    void check_expr_for_uninit(Expr* expr, const unordered_set<string>& initVars, vector<int>& warnings);
    // Computes the degree of a polynomial from its AST.
    int computeDegree(Expr* expr);
    // Outputs the degrees of all declared polynomials.
    void outputPolynomialDegrees();
};

#endif // __PARSER_H__
