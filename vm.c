#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include "common.h"
#include "vm.h"
#include "debug.h"
#include "compiler.h"
#include "object.h"

static void resetStack(VM* vm) {
    vm->stack.count = 0;
    vm->frameCount = 0;
    vm->openUpvalues = NULL;
}

static void runtimeError(VM* vm, const char* format, ...) {
    fprintf(stderr, "\n\033[1;31m");
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int32_t i = vm->frameCount - 1; i >= 0; i--) {
        CallFrame* frame = vm->frames + i;
        size_t instruction = frame->ip - frame->closure->function->chunk.code - 1;
        uint32_t line = getLine(&frame->closure->function->chunk, instruction);
        fprintf(stderr, "\033[1;31m[line %d] in ", line);
        if (!frame->closure->function->name) {
            fprintf(stderr, "script\n\033[0m");
        } else {
            fprintf(stderr, "%s()\n\033[0m", frame->closure->function->name->chars);
        }
    }

    resetStack(vm);
}

static bool clockNative(UNUSED VM* vm, Value* out, UNUSED Value* args) {
    *out = NUMBER_VAL((double) clock() / CLOCKS_PER_SEC);
    return true;
}

static bool sqrtNative(VM* vm, Value* out, Value* args) {
    if (IS_NUMBER(*args)) {
        *out = NUMBER_VAL(sqrt(AS_NUMBER(*args)));
        return true;
    } else {
        runtimeError(vm, "Type error.");
        return false;
    }
}

static void defineNative(VM* vm, const char* name, NativeFn function, uint8_t arity) {
    push(vm, OBJ_VAL(copyString(vm, NULL, name, strlen(name))));
    push(vm, OBJ_VAL(newNative(vm, NULL, function, arity)));
    tableSet(vm, NULL, &vm->globals, AS_STRING(vm->stack.values[0]), vm->stack.values[1]);
    pop(vm);
    pop(vm);
}

void initVM(FreeList* freeList, VM* vm) {
    vm->freeList = freeList;
    resetStack(vm);
    vm->objects = NULL;
    initTable(&vm->globals);
    initTable(&vm->strings);
    vm->print = printf;
    vm->greyCount = 0;
    vm->greyCapacity = 0;
    vm->greyStack = NULL;
    vm->bytesAllocated = 0;
    vm->nextGC = 1024 * 1024;
    vm->initString = NULL;
    initValueArray(vm, NULL, &vm->stack);
    vm->initString = copyString(vm, NULL, "init", 4);

    defineNative(vm, "clock", clockNative, 0);
    defineNative(vm, "sqrt", sqrtNative, 1);
}

void freeVM(VM* vm) {
    freeTable(vm, &vm->globals);
    freeTable(vm, &vm->strings);
    freeValueArray(vm, &vm->stack);
    vm->initString = NULL;
    freeObjects(vm);
    // use system allocator as the custom allocator depends on this
    free(vm->greyStack);
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static bool call(VM* vm, ObjClosure* closure, uint8_t argumentCount) {
    if (argumentCount != closure->function->arity) {
        runtimeError(vm, "Expected %d arguments but got %d.", closure->function->arity, argumentCount);
        return false;
    }

    if (vm->frameCount == FRAMES_MAX) {
        runtimeError(vm, "Stack overflow.");
        return false;
    }

    CallFrame* frame = vm->frames + vm->frameCount++;
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->base = vm->stack.count - argumentCount - 1;
    return true;
}

static bool callValue(VM* vm, Value callee, uint8_t argumentCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                vm->stack.values[vm->stack.count - argumentCount - 1] = bound->receiver;
                return call(vm, bound->method, argumentCount);
            }
            case OBJ_CLASS: {
                ObjClass* class = AS_CLASS(callee);
                vm->stack.values[vm->stack.count - argumentCount - 1] = OBJ_VAL(newInstance(vm, NULL, class));
                // invoke constructor, if it exists
                Value initialiser;
                if (tableGet(&class->methods, vm->initString, &initialiser)) {
                    return call(vm, AS_CLOSURE(initialiser), argumentCount);
                } else if (argumentCount != 0) {
                    runtimeError(vm, "Expected 0 arguments but got %d.", argumentCount);
                    return false;
                }
                return true;
            }
            case OBJ_CLOSURE:
                return call(vm, AS_CLOSURE(callee), argumentCount);
            case OBJ_NATIVE: {
                ObjNative* native = AS_NATIVE(callee);
                if (argumentCount != native->arity) {
                    runtimeError(vm, "Expected %d arguments but got %d", native->arity, argumentCount);
                    return false;
                }
                Value result;
                bool successful = native->function(vm, &result, vm->stack.values + vm->stack.count - argumentCount);
                if (successful) {
                    vm->stack.count -= argumentCount + 1;
                    push(vm, result);
                    return true;
                } else {
                    return false;
                }
            }
            default:
                break;
        }
    }
    runtimeError(vm, "Can only call functions and classes.");
    return false;
}

static Value peek(VM* vm, uint32_t distance) {
    return vm->stack.values[vm->stack.count - 1 - distance];
}

static void concatenate(VM* vm) {
    // keep on stack so GC can reach
    ObjString* b = AS_STRING(peek(vm, 0));
    ObjString* a = AS_STRING(peek(vm, 1));

    uint32_t length = a->length + b->length;
    char* chars = VM_ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(vm, NULL, chars, length);
    pop(vm);
    pop(vm);
    push(vm, OBJ_VAL(result));
}

static ObjUpvalue* captureUpvalue(VM* vm, Value* local) {
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm->openUpvalues;

    while (upvalue && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = newUpvalue(vm, NULL, local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue) {
        prevUpvalue->next = createdUpvalue;
    } else {
        vm->openUpvalues = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(VM* vm, Value* last) {
    while (vm->openUpvalues && vm->openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm->openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm->openUpvalues = upvalue->next;
    }
}

static void defineMethod(VM* vm, ObjString* name) {
    Value method = peek(vm, 0);
    ObjClass* class = AS_CLASS(peek(vm, 1));
    tableSet(vm, NULL, &class->methods, name, method);
    pop(vm);
}

static bool bindMethod(VM* vm, ObjClass* class, ObjString* name) {
    Value method;
    if (!tableGet(&class->methods, name, &method)) {
        // TODO this is a bit harsh in a dynamic language - maybe print warning and return nil instead
        runtimeError(vm, "Undefined property '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod* bound = newBoundMethod(vm, NULL, peek(vm, 0), AS_CLOSURE(method));
    pop(vm); // instance
    push(vm, OBJ_VAL(bound));
    return true;
}

uint8_t readByte(CallFrame* frame) {
    uint8_t result = *frame->ip;
    ++frame->ip;
    return result;
}

static bool isWide(OpCode op) {
    return op == OP_GET_GLOBAL_LONG || op == OP_SET_GLOBAL_LONG || op == OP_CONSTANT_LONG ||
           op == OP_DEFINE_GLOBAL_LONG || op == OP_GET_LOCAL_LONG || op == OP_SET_LOCAL_LONG;
}

static bool invokeFromClass(VM* vm, ObjClass* class, ObjString* name, uint8_t argumentCount) {
    Value method;
    if (!tableGet(&class->methods, name, &method)) {
        runtimeError(vm, "Undefined property '%s'.", name->chars);
        return false;
    }
    return call(vm, AS_CLOSURE(method), argumentCount);
}

static bool invoke(VM* vm, ObjString* name, uint8_t argumentCount) {
    Value receiver = peek(vm, argumentCount);

    if (!IS_INSTANCE(receiver)) {
        runtimeError(vm, "Only instances have methods.");
        return false;
    }

    ObjInstance* instance = AS_INSTANCE(receiver);

    // something that looks like a method call could actually be invoking a function stored in a field
    Value value;
    if (tableGet(&instance->fields, name, &value)) {
        vm->stack.values[vm->stack.count - argumentCount - 1] = value;
        return callValue(vm, value, argumentCount);
    }

    return invokeFromClass(vm, instance->class, name, argumentCount);
}

static InterpretResult run(VM* vm) {
    CallFrame* frame = vm->frames + vm->frameCount - 1;

// TODO (maybe) store the ip in a register - need to ensure the ip is stored/loaded properly when the frame changes
#define READ_BYTE (readByte(frame))
#define READ_LONG ((READ_BYTE << 16) | (READ_BYTE << 8) | READ_BYTE)
#define READ_SHORT ((READ_BYTE << 8) | (READ_BYTE))
#define PEEK(distance) peek(vm, distance)
#define BINARY_OP(valueType, op) do { \
    if (!IS_NUMBER(PEEK(0)) || !IS_NUMBER(PEEK(1))) { \
        runtimeError(vm, "Operands must be numbers.");\
        return INTERPRET_RUNTIME_ERROR; \
    }\
    double b = AS_NUMBER(pop(vm));    \
    double a = AS_NUMBER(PEEK(0));    \
    vm->stack.values[vm->stack.count - 1] = valueType(a op b);\
} while (false)
#define READ_CONSTANT(index) (frame->closure->function->chunk.constants.values[index])
#define READ_STRING(index) AS_STRING(READ_CONSTANT(index))

    while (frame->ip < frame->closure->function->chunk.code + frame->closure->function->chunk.count) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (uint32_t i = 0; i < vm->stack.count; i++) {
            printf("[");
            printValue(printf, vm->stack.values[i]);
            printf("]");
        }
        printf("\n");
        disassembleInstruction(&frame->closure->function->chunk, (uint32_t) (frame->ip - frame->closure->function->chunk.code));
#endif
        OpCode instruction;
        switch (instruction = READ_BYTE) {
            case OP_PRINT:
                printValue(vm->print, pop(vm));
                printf("\n");
                break;
            case OP_POP:
                pop(vm);
                break;
            case OP_ADD: {
                if (IS_STRING(PEEK(0)) && IS_STRING(PEEK(1))) {
                    concatenate(vm);
                } else if (IS_NUMBER(PEEK(0)) && IS_NUMBER(PEEK(1))) {
                    double b = AS_NUMBER(pop(vm));
                    double a = AS_NUMBER(PEEK(0));
                    vm->stack.values[vm->stack.count - 1] = NUMBER_VAL(a + b);
                } else {
                    runtimeError(vm, "Operands must be two numbers or two strings");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: {
                BINARY_OP(NUMBER_VAL, -);
                break;
            }
            case OP_MULTIPLY: {
                BINARY_OP(NUMBER_VAL, *);
                break;
            }
            case OP_DIVIDE: {
                BINARY_OP(NUMBER_VAL, /);
                break;
            }
            case OP_NEGATE: {
                if (!IS_NUMBER(PEEK(0))) {
                    runtimeError(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(vm, NUMBER_VAL(-AS_NUMBER(pop(vm))));
                break;
            }
            case OP_CONSTANT:
            case OP_CONSTANT_LONG: {
                uint32_t index = isWide(instruction) ? READ_LONG : READ_BYTE;
                push(vm, READ_CONSTANT(index));
                break;
            }
            case OP_DEFINE_GLOBAL:
            case OP_DEFINE_GLOBAL_LONG: {
                uint32_t index = isWide(instruction) ? READ_LONG : READ_BYTE;
                ObjString* name = READ_STRING(index);
                tableSet(vm, NULL, &vm->globals, name, PEEK(0));
                pop(vm);
                break;
            }
            case OP_GET_GLOBAL:
            case OP_GET_GLOBAL_LONG: {
                uint32_t index = isWide(instruction) ? READ_LONG : READ_BYTE;
                ObjString* name = READ_STRING(index);
                Value value;
                if (!tableGet(&vm->globals, name, &value)) {
                    runtimeError(vm, "Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(vm, value);
                break;
            }
            case OP_SET_GLOBAL:
            case OP_SET_GLOBAL_LONG: {
                uint32_t index = isWide(instruction) ? READ_LONG : READ_BYTE;
                ObjString* name = READ_STRING(index);
                if (tableSet(vm, NULL, &vm->globals, name, PEEK(0))) {
                    tableDelete(&vm->globals, name);
                    runtimeError(vm, "Undefined variable '%s'", name);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_LOCAL:
            case OP_GET_LOCAL_LONG: {
                uint32_t index = isWide(instruction) ? READ_LONG : READ_BYTE;
                push(vm, vm->stack.values[frame->base + index]);
                break;
            }
            case OP_SET_LOCAL:
            case OP_SET_LOCAL_LONG: {
                uint32_t index = isWide(instruction) ? READ_LONG : READ_BYTE;
                vm->stack.values[frame->base + index] = PEEK(0);
                break;
            }
            case OP_NIL: {
                push(vm, NIL_VAL);
                break;
            }
            case OP_TRUE: {
                push(vm, BOOL_VAL(true));
                break;
            }
            case OP_FALSE: {
                push(vm, BOOL_VAL(false));
                break;
            }
            case OP_NOT: {
                push(vm, BOOL_VAL(isFalsey(pop(vm))));
                break;
            }
            case OP_EQUAL: {
                Value b = pop(vm);
                vm->stack.values[vm->stack.count - 1] = BOOL_VAL(valuesEqual(PEEK(0), b));
                break;
            }
            case OP_GREATER: {
                BINARY_OP(BOOL_VAL, >);
                break;
            }
            case OP_LESS: {
                BINARY_OP(BOOL_VAL, <);
                break;
            }
            case OP_JUMP: {
                uint32_t offset = READ_SHORT;
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint32_t offset = READ_SHORT;
                if (isFalsey(PEEK(0))) frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                uint32_t offset = READ_SHORT;
                frame->ip -= offset;
                break;
            }
            case OP_CALL: {
                uint8_t argumentCount = READ_BYTE;
                if (!callValue(vm, PEEK(argumentCount), argumentCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = vm->frames + vm->frameCount - 1;
                break;
            }
            case OP_CLASS: {
                push(vm, OBJ_VAL(newClass(vm, NULL, READ_STRING(READ_BYTE))));
                break;
            }
            case OP_CLOSURE: {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT(READ_BYTE));
                ObjClosure* closure = newClosure(vm, NULL, function);
                push(vm, OBJ_VAL(closure));
                for (uint32_t i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE;
                    uint8_t index = READ_BYTE;

                    if (isLocal) {
                        closure->upvalues[i] = captureUpvalue(vm, vm->stack.values + frame->base + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE;
                push(vm, *frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE;
                *frame->closure->upvalues[slot]->location = PEEK(0);
                break;
            }
            case OP_GET_UPVALUE_LONG:
            case OP_SET_UPVALUE_LONG:
                assert(!"Not implemented");
            case OP_CLOSE_UPVALUE: {
                closeUpvalues(vm, vm->stack.values + vm->stack.count - 1);
                pop(vm);
                break;
            }
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(vm, 0))) {
                    runtimeError(vm, "Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = AS_INSTANCE(peek(vm, 0));
                ObjString* name = READ_STRING(READ_BYTE);

                Value value;
                if (tableGet(&instance->fields, name, &value)) {
                    pop(vm); // instance
                    push(vm, value);
                    break;
                }

                if (!bindMethod(vm, instance->class, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(vm, 1))) {
                    runtimeError(vm, "Only instances have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = AS_INSTANCE(peek(vm, 1));
                tableSet(vm, NULL, &instance->fields, READ_STRING(READ_BYTE), peek(vm, 0));
                Value value = pop(vm);
                pop(vm); // instance
                push(vm, value);
                break;
            }
            case OP_METHOD: {
                defineMethod(vm, READ_STRING(READ_BYTE));
                break;
            }
            case OP_INVOKE: {
                ObjString* method = READ_STRING(READ_BYTE);
                uint8_t argumentCount = READ_BYTE;
                if (!invoke(vm, method, argumentCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = vm->frames + vm->frameCount - 1;
                break;
            }
            case OP_INHERIT: {
                Value superclass = peek(vm, 1);
                if (!IS_CLASS(superclass)) {
                    runtimeError(vm, "Superclass must be a class.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjClass* subclass = AS_CLASS(peek(vm, 0));
                tableAddAll(vm, NULL, &AS_CLASS(superclass)->methods, &subclass->methods);
                pop(vm); // subclass
                break;
            }
            case OP_GET_SUPER: {
                ObjString* name = READ_STRING(READ_BYTE);
                ObjClass* superclass = AS_CLASS(pop(vm));

                if (!bindMethod(vm, superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_RETURN: {
                Value result = pop(vm);
                closeUpvalues(vm, vm->stack.values + frame->base);
                if (--vm->frameCount == 0) {
                    pop(vm);
                    return INTERPRET_OK;
                }

                vm->stack.count = frame->base;
                push(vm, result);
                frame = vm->frames + vm->frameCount - 1;
                break;
            }
        }
    }

    return INTERPRET_OK;

#undef READ_BYTE
#undef READ_SHORT
#undef READ_LONG
#undef PEEK
#undef BINARY_OP
}

InterpretResult interpret(VM* vm, const char* source) {
    ObjFunction* function = compile(vm, source);
    if (!function) return INTERPRET_COMPILE_ERROR;

    push(vm, OBJ_VAL(function));
    ObjClosure* closure = newClosure(vm, NULL, function);
    pop(vm);
    push(vm, OBJ_VAL(closure));
    call(vm, closure, 0);

    return run(vm);
}

void push(VM* vm, Value value) {
    writeValue(vm, NULL, &vm->stack, value);
}

Value pop(VM* vm) {
    if (vm->stack.count <= 0) assert(!"Empty stack");

    return vm->stack.values[--vm->stack.count];
}

void markRoots(VM* vm) {
    for (uint32_t i = 0; i < vm->stack.count; i++) {
        markValue(vm, vm->stack.values[i]);
    }

    for (uint32_t i = 0; i < vm->frameCount; i++) {
        markObject(vm, (Obj*) vm->frames[i].closure);
    }

    for (ObjUpvalue* upvalue = vm->openUpvalues; upvalue; upvalue = upvalue->next) {
        markObject(vm, (Obj*) upvalue);
    }

    markTable(vm, &vm->globals);
    markObject(vm, (Obj*) vm->initString);
}
