# XVM
General purpose cross-platform customizable scripting language runtime

# Examples

```cpp
#include <XVM/xvm.h>

int main() {
    using enum xvm::Opcode;

    xvm::ConstantHolder k_holder;
    xvm::BytecodeHolder bc_holder;

    // Name of the global print function
    k_holder.pushConstant("print");

    // Print message
    k_holder.pushConstant("Hello, world!");

    // Load "print" into register 0
    bc_holder.pushInstruction(LOADK, {0, 0});

    // Load the print function into register 1
    bc_holder.pushInstruction(GETGLOBAL, {1, 0});

    // Push the print message onto the stack
    bc_holder.pushInstruction(PUSHK, {1});

    // Call the print function
    bc_holder.pushInstruction(CALL, {1});

    // Exit program
    bc_holder.pushInstruction(RETNIL);

    // Stack register file. This is required because it is conventionally too large to fit inside the state object.
    xvm::StkRegFile file;

    // State object.
    xvm::State state(k_holder, bc_holder, file);
    
    // Execute the program
    xvm::execute(state); // Hello, world!

    return 0;
}
```
