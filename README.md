# XVM
General purpose cross-platform customizable scripting language runtime

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
