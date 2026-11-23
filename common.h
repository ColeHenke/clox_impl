#ifndef CLOX_IMPL_COMMON_H
#define CLOX_IMPL_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEBUG_PRINT_CODE

// with this enabled, the vm will disassemble each instruction before execution
// it does this on the fly
#define DEBUG_TRACE_EXECUTION

// the instruction operand used to encode is one byte - so the vm has a limit on how many locals that can be in scope
//    at the same time - therefore the locals array has a fixed size
#define UINT8_COUNT (UINT8_MAX + 1)

#endif //CLOX_IMPL_COMMON_H
