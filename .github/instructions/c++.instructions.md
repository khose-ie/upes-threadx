---
name: 'C/C++ Code Standards'
description: 'Condensed coding conventions for C and C++ files'
applyTo: '**/*.c, **/*.h, **/*.cc, **/*.hh, **/*.cpp, **/*.hpp, **/*.cxx, **/*.hxx'
---

# C/C++ Code Standards

- Scope: Third-party or directly included open-source code is exempt.
- Naming: Files and directories must be lowercase and use `-` as separator. Repeat the module name in the directory and filename to avoid name collisions (e.g. `module-a/module-a-submodule1.c`).
- File header: Required Doxygen block with `@file`, `@author`, `@date`, `@brief`.

- Indentation & whitespace: Tabs are forbidden (except in CMake/Makefiles). Use 4 spaces. Maximum indentation depth is 3 levels; if exceeded, add a single-line comment inside the function explaining why.
- Braces: Opening brace must start on a new line (applies to functions and control statements: `if`, `for`, `while`, `switch`).
- Line & function length: Max line length 100 characters. Max function length 100 lines; if exceeded, add a one-line comment inside the function explaining the reason.

- Comments: Use Doxygen style for files, functions, classes, structs, enums and macros.
- Macros: Macro values use UPPER_SNAKE_CASE. Macro functions use lower_snake_case (per project preference). Always wrap macro values in parentheses. Avoid context-dependent macro functions.
- Constants: Literal numeric constants other than `0` must be defined as macros and not embedded directly in function logic.

- Naming conventions: variables and function parameters use lower_snake_case; global variable name start with a `_`; function names use lower_snake_case; type names (classes/structs/unions/enums) use PascalCase and camelCase and must end with `_t`, if includes module name, it should be cameCase (e.g. `TheType_t` or `moduleType_t`); member fields use lower_snake_case. Avoid using overly short names even in local scope or loops.
- Enums: C-style enum members use UPPER_SNAKE_CASE and should be prefixed with the enum name in uppercase (e.g. `COLOR_RED`). `enum class` members use PascalCase.
- Syntax & spacing: Operators must have spaces on both sides. Parenthesize sub-expressions in compound conditional expressions. Place a space between control keywords and the following `(`. Control blocks must use `{}`. In `switch`, `case` labels are not indented; the case contents and the `break` are indented.
- Limits & prohibited patterns: `goto` is forbidden (use `do { ... } while(0)` if necessary). Function parameter count should not exceed 5; if exceeded, add a one-line comment explaining why.
