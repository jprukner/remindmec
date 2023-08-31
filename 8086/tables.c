#include "simulate_8086_ISA.h"
#include <stdio.h>

#define REGISTER_COUNT 8

static const char *register_word_map[][REGISTER_COUNT] = {
    {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"},
    {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"},
};

static const char *size_modifiers[] = {"byte", "word"};

static const char *accumulator_registers[] = {"al", "ax"};

static const char *memory_displacement_expresion_table[][8] = {
    {
        "[bx + si]",
        "[bx + di]",
        "[bp + si]",
        "[bp + di]",
        "[si]",
        "[di]",
        "[%u]",
        "[bx]",
    },
    {
        "[bx + si + %u]",
        "[bx + di + %u]",
        "[bp + si + %u]",
        "[bp + di + %u]",
        "[si + %u]",
        "[di + %u]",
        "[bp + %u]",
        "[bx + %u]",
    },
    {
        "[bx + si + %u]",
        "[bx + di + %u]",
        "[bp + si + %u]",
        "[bp + di + %u]",
        "[si + %u]",
        "[di + %u]",
        "[bp + %u]",
        "[bx + %u]",
    },
};

struct single_byte_prefix_instruction
    instruction_opcode_single_byte_prefixes[] = {
        // mov immmediate to register
        {.mask = 0b11110000, .value = 0b10110000},
        // mov reg reg/mem with optional displacement
        {.mask = 0b11111100, .value = 0b10001000},
        // add immediate to accumulator
        {.mask = 0b11111110, .value = 0b00000100},
        // add reg reg/mem with optional displacement
        {.mask = 0b11111100, .value = 0b00000000},
        // sub immediate from accumulator
        {.mask = 0b11111110, .value = 0b00101100},
        // sub reg reg/mem with optional displacement
        {.mask = 0b11111100, .value = 0b00101000},
        // cmp immediate and accumulator
        {.mask = 0b11111110, .value = 0b00111100},
        // cmp reg and reg/mem with optional displacement
        {.mask = 0b11111100, .value = 0b00111000},
        // jnz
        {.mask = 0b11111111, .value = 0b01110101},
        // jl
        {.mask = 0b11111111, .value = 0b01111100},
        // jle
        {.mask = 0b11111111, .value = 0b01111110},
        // jb
        {.mask = 0b11111111, .value = 0b01110010},
        // jbe
        {.mask = 0b11111111, .value = 0b01110110},
        // jp
        {.mask = 0b11111111, .value = 0b01111010},
        // jo
        {.mask = 0b11111111, .value = 0b01110000},
        // js
        {.mask = 0b11111111, .value = 0b01111000},
        // jnl
        {.mask = 0b11111111, .value = 0b01111101},
        // jg
        {.mask = 0b11111111, .value = 0b01111111},
        // jnb
        {.mask = 0b11111111, .value = 0b01110011},
        // ja
        {.mask = 0b11111111, .value = 0b01110111},
        // jnp
        {.mask = 0b11111111, .value = 0b01111011},
        // jno
        {.mask = 0b11111111, .value = 0b01110001},
        // jns
        {.mask = 0b11111111, .value = 0b01111001},
        // loop
        {.mask = 0b11111111, .value = 0b11100010},
        // loopz
        {.mask = 0b11111111, .value = 0b11100001},
        // loopnz
        {.mask = 0b11111111, .value = 0b11100000},
        // jcxz
        {.mask = 0b11111111, .value = 0b11100011},
        // jz
        {.mask = 0b11111111, .value = 0b01110100},

};
struct two_byte_prefix_instruction instruction_opcode_two_byte_prefixes[] = {
    // mov immediate to memory/reg with optional displacement
    {.mask = 0b1111111000111000, .value = 0b1100011000000000},

    // add immmediate to register/memory
    {.mask = 0b1111110000111000, .value = 0b1000000000000000},

    // sub immmediate from register/memory
    {.mask = 0b1111110000111000, .value = 0b1000000000101000},

    // cmp immmediate and register/memory
    {.mask = 0b1111110000111000, .value = 0b1000000000111000},
};

char *instruction_names_single_byte_prefix[] = {
    "mov", "mov", "add", "add",  "sub",   "sub",    "cmp",  "cmp", "jnz", "jl",
    "jle", "jb",  "jbe", "jp",   "jo",    "js",     "jnl",  "jg",  "jnb", "ja",
    "jnp", "jno", "jns", "loop", "loopz", "loopnz", "jcxz", "jz",
};

char *instruction_names_two_byte_prefix[] = {
    "mov",
    "add",
    "sub",
    "cmp",
};

int (*single_byte_instruction_prefix_decoders[])(
    struct context *ctx, uint8_t instruction_byte, FILE *executable,
    const char *instruction_name) = {
    decode_instruction_immediate_to_reg,         // mov bx, 6
    decode_instruction_reg_mem_reg,              // mov ax, [bp + 2]
    decode_instruction_immediate_to_accumulator, // add ax, 7
    decode_instruction_reg_mem_reg,              // add ax, [bp +2]
    decode_instruction_immediate_to_accumulator, // sub ax, 7
    decode_instruction_reg_mem_reg,              // sub ax, [bp +2]
    decode_instruction_immediate_to_accumulator, // cmp ax, 7
    decode_instruction_reg_mem_reg,              // cmp ax, [bp +2]
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
    decode_instruction_jump,
};

int (*two_byte_instruction_prefix_decoders[])(struct context *ctx,
                                              uint16_t instruction_bytes,
                                              FILE *executable,
                                              const char *instruction_name) = {
    decode_instruction_immediate_to_memory_reg, // mov [bp + 2], 7
    decode_instruction_immediate_to_memory_reg, // add [bp + 2], 7
    decode_instruction_immediate_to_memory_reg, // sub [bp + 2], 7
    decode_instruction_immediate_to_memory_reg, // cmp [bp + 2], 7
};
