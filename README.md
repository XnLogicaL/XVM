# XVM
General purpose cross-platform customizable scripting language runtime

# Features

## Performance

- XVM performs around 1.5-2x better than Lua on average. (TODO: ADD BENCH)
- XVM utilizes C++ RAII in a performant way to ensure maximum memory efficiency and minimal allocations/deallocations to ensure maximum performance during runtime.

## Register/stack hybrid interpreter

XVM utilizes a register-based bytecode format, while using the stack for allocation variables and calling functions.

## Advanced C++ interoperability

XVM is written in C++23, and has an advanced API to make it as embedable as possible. The C++ API is capable of all functionality (and more) that the bytecode interpreter is, as they use the same internal API.

## Efficient data types

XVM features the following data types as built-in values:
- Integer (32-bit)
- IEEE-754 Float (32-bit) 
- Boolean
- String
- Array
- Dictionary
- Function/closure (native functions included)

## CISC-like bytecode instruction set

XVM features a quite sizable instruction set with 16-bit addressable opcodes. It has a lot of combination opcodes like `JMPIFGTEQ` (jump to address if greater than or equal to) and `IADD` (add integer to value in register).

# Examples

```cpp
#include <vector>
#include <XVM/xvm.h>

int main() {
    using enum xvm::Opcode;

    std::vector<xvm::Value> k_holder;
    std::vector<xvm::Instruction> bcHolder;
    std::vector<xvm::InstructionData> bcInfoHolder;

    // Name of the global print function
    k_holder.emplace_back("print");

    // Print message
    k_holder.emplace_back("Hello, world!");

    // Load "print" into register 0
    bcHolder.emplace_back(LOADK, 0, 0);

    // Load the print function into register 1
    bcHolder.emplace_back(GETGLOBAL, 1, 0);

    // Push the print message onto the stack
    bcHolder.emplace_back(PUSHK, 1);

    // Call the print function
    bcHolder.emplace_back(CALL, 1);

    // Exit program
    bcHolder.emplace_back(RETNIL);

    // Stack register file. This is required because it is conventionally too large to fit inside the state object.
    xvm::StkRegFile file;
    xvm::State state(k_holder, bcHolder, bcInfoHolder, file);
    
    // Execute the program
    xvm::execute(state); // Hello, world!

    return 0;
}
```
