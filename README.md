# PolyCalc Compiler

A recursive descent compiler for a domain-specific polynomial language that performs lexical analysis, syntax parsing, semantic checking (including duplicate declarations, undeclared variables, uninitialized variables, and useless assignments), execution, and polynomial degree calculation.

## Overview

PolyCalc Compiler compiles and executes a simple polynomial language. Key features include:
- Lexical Analysis: Tokenization via an external lexer and buffered input system.
- Syntax Parsing: Recursive descent parsing to construct an Abstract Syntax Tree (AST) for polynomial expressions.
- Semantic Analysis: Comprehensive error checking (syntax, semantic errors, and warnings such as uninitialized variables and useless assignments).
- Execution Engine: Memory allocation and polynomial evaluation with correct arithmetic semantics.
- Polynomial Degree Calculation: Determination and output of the degree for each declared polynomial.

## File Structure

Developed Components:
- **parser.h / parser.cc**  
  Implements the recursive descent parser, semantic checks, execution engine, and degree calculation.

External Components (not developed by me):
- **lexer.h / lexer.cc**  
  Implements the lexical analyzer for tokenizing the input.
- **inputbuf.h / inputbuf.cc**  
  Provides buffered input operations.

## Compilation

Assuming all required files are available, compile the project using:

    g++ -o polycalc lexer.cc parser.cc inputbuf.cc -std=c++11

## Usage

Run the executable with an input file containing the source code:

    ./polycalc <input_file>
