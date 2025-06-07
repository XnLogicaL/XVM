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

## Runtime label addressing

XVM features a label address table that can be used to address labels during runtime. Labels are "declared" using the special `LBL` opcode, which is completely identical to a NOP instruction.

## CISC-like bytecode instruction set

XVM features a quite sizable instruction set with 16-bit addressable opcodes. It has a lot of combination opcodes like `LJMPIFGTEQ` (jump to label if greater than or equal to) and `IADD` (add integer to value in register).

# Examples

```cpp
#include <vector>
#include <XVM/xvm.h>

int main() {
    using enum xvm::Opcode;

    std::vector<xvm::Value> k_holder;
    std::vector<xvm::Instruction> bc_holder;
    std::vector<xvm::InstructionData> bc_info_holder;

    // Name of the global print function
    k_holder.emplace_back("print");

    // Print message
    k_holder.emplace_back("Hello, world!");

    // Load "print" into register 0
    bc_holder.emplace_back(LOADK, 0, 0);

    // Load the print function into register 1
    bc_holder.emplace_back(GETGLOBAL, 1, 0);

    // Push the print message onto the stack
    bc_holder.emplace_back(PUSHK, 1);

    // Call the print function
    bc_holder.emplace_back(CALL, 1);

    // Exit program
    bc_holder.emplace_back(RETNIL);

    // Stack register file. This is required because it is conventionally too large to fit inside the state object.
    xvm::StkRegFile file;
    xvm::State state(k_holder, bc_holder, bc_info_holder, file);
    
    // Execute the program
    xvm::execute(state); // Hello, world!

    return 0;
}
```
