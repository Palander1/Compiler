# Compiler Components Project

This repository contains an implementation of a compiler's core components. These components are used for lexical analysis and syntax parsing of input source code. However, **some essential files** required for full functionality are not included in this repository as they are not owned by me.

## File Structure
Written by me:  
- **parser.h / parser.cc** - Implementation of the syntax analyzer (parser), responsible for constructing an abstract syntax tree (AST) from tokens.

Not written by me and not included in the repo:  
- **lexer.h / lexer.cc** - Implementation of the lexical analyzer (lexer), responsible for tokenizing the input source code.
- **inputbuf.h / inputbuf.cc** - Handles buffered input operations.

## Compilation

If all required files are available, you can compile the project using g++:

```sh
g++ -o compiler lexer.cc parser.cc inputbuf.cc -std=c++11
Usage
If compiled successfully, the executable can be run as follows:

./compiler <input_file>
where <input_file> is the source code to be analyzed.
