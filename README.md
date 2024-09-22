# `runml`: A Mini-Language Compiler

## Project Overview

**runml** is a C11 program designed to transpile and execute a program written in a mini-language, **ml**. Unlike larger programming languages such as Python, Java, or C, mini-languages are often embedded in other programs and lack the full-fledged support of standard libraries and modules.

This project introduces **ml**, a simple mini-language, which is transpiled into C code and then compiled using a C compiler. The compiled program is then executed with any optional command-line arguments provided. This process allows for execution of **ml** code without building a full-fledged interpreter.

## Table of Contents
- [Introduction](#introduction)
- [Installation](#installation)
- [Usage](#usage)
- [ml Language Specification](#ml-language-specification)
- [Working](#working)
- [Project Requirements](#project-requirements)
- [Future Work](#future-work)

## Introduction

In **runml**, we aim to:

- Validate **ml** programs by checking their syntax.
- Transpile valid **ml** code into C11 code.
- Compile the transpiled C code using the system’s C compiler.
- Execute the compiled C program, passing any optional command-line arguments.
  
The transpilation from **ml** to C leverages the extensive C toolchain, allowing **C** to act as a "high-level assembly language."

## Installation

To install and run the **runml** program, you need:

- A C11-compatible compiler (e.g., GCC or Clang)
- A Unix-like environment (Linux, macOS)

### Build Instructions

1. Clone this repository:
   ```bash
   git clone https://github.com/niranjan-v2/mini-lang-transpiler
   ```
2. Navigate to the project directory:
   ```bash
   cd mini-lang-transpiler
   ```
3. Build the program:
   ```bash
   make
   ```
## Usage
To run **`runml`**, use the following syntax:
  ```bash
    ./runml <ml-program-file> [optional-arguments]
  ```
- `<ml-program-file>`: Path to a text file containing **ml** code.
- `[optional-arguments]`: Real-number arguments passed to the compiled program.
  
Example:

  ```bash
  ./runml sample01.ml 3.14159
  ```
## ml Language Specification
- **File format**: Programs are written in text files with a `.ml` extension.
- **Comments**: Lines starting with `#` are comments.
- **Data types**: The only data type supported is real numbers (e.g., `2.71828`).
- **Identifiers**: Consist of 1-12 lowercase alphabetic characters (e.g., `budgie`).
- **Variables**: Automatically initialized to `0.0` and do not need explicit declaration.
- **Arguments**: Variables like `arg0`, `arg1`, etc., hold command-line arguments.
- **Functions**: Must be defined before use, have zero or more parameters, and are indented with tabs.
## Working
**`runml`** will:
- Validate the **ml** code and report any syntax errors.
- Generate a C11 file (e.g., `ml-12345.c`).
- Compile the generated C file.
- Execute the compiled C program, passing any command-line arguments.
- Syntax errors will be redirected to `stderr` with a leading `!` character.
- Sends the output of the code to `stdout` and all the temporary files generated during execution will be automatically removed.
## Project Requirements
- **Source file**: The project must be written in a single source file named `runml.c`.
- **No third-party dependencies**: Only system-provided libraries are allowed.
- **Output formatting**:
  - Exact integers are printed without decimal places.
  - Non-integer real numbers are printed with exactly 6 decimal places.
## Future Work
Future enhancements could include adding:
- Conditional statements
- Loops
- Additional data types
 
