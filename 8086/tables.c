#include "simulate_8086_ISA.h"
#include <stdint.h>
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

struct instruction instructions_single_byte_prefix[] = {
    // mov bx, 6
    {.id = MOV, .name = "mov", .decoder = decode_instruction_immediate_to_reg},

    // mov ax, [bp + 2]
    {.id = MOV, .name = "mov", .decoder = decode_instruction_reg_mem_reg},

    // add ax, 7
    {
        .id = ADD,
        .name = "add",
        .decoder = decode_instruction_immediate_to_accumulator,
    },

    // add ax, [bp +2]
    {.id = ADD, .name = "add", .decoder = decode_instruction_reg_mem_reg},

    // sub ax, 7
    {
        .id = SUB,
        .name = "sub",
        .decoder = decode_instruction_immediate_to_accumulator,
    },

    // sub ax, [bp +2]
    {.id = SUB, .name = "sub", .decoder = decode_instruction_reg_mem_reg},

    // cmp ax, 7
    {
        .id = CMP,
        .name = "cmp",
        .decoder = decode_instruction_immediate_to_accumulator,
    },

    // cmp ax, [bp +2]
    {.id = CMP, .name = "cmp", .decoder = decode_instruction_reg_mem_reg},

    {.id = JNZ, .name = "jnz", .decoder = decode_instruction_jump},
    {.id = JL, .name = "jl", .decoder = decode_instruction_jump},
    {.id = JLE, .name = "jle", .decoder = decode_instruction_jump},
    {.id = JB, .name = "jb", .decoder = decode_instruction_jump},
    {.id = JBE, .name = "jbe", .decoder = decode_instruction_jump},
    {.id = JP, .name = "jp", .decoder = decode_instruction_jump},
    {.id = JO, .name = "jo", .decoder = decode_instruction_jump},
    {.id = JS, .name = "js", .decoder = decode_instruction_jump},
    {.id = JNL, .name = "jnl", .decoder = decode_instruction_jump},
    {.id = JG, .name = "jg", .decoder = decode_instruction_jump},
    {.id = JNB, .name = "jnb", .decoder = decode_instruction_jump},
    {.id = JA, .name = "ja", .decoder = decode_instruction_jump},
    {.id = JNP, .name = "jnp", .decoder = decode_instruction_jump},
    {.id = JNO, .name = "jno", .decoder = decode_instruction_jump},
    {.id = JNS, .name = "jns", .decoder = decode_instruction_jump},
    {.id = LOOP, .name = "loop", .decoder = decode_instruction_jump},
    {.id = LOOPZ, .name = "loopz", .decoder = decode_instruction_jump},
    {.id = LOOPNZ, .name = "loopnz", .decoder = decode_instruction_jump},
    {.id = JCXZ, .name = "jcxz", .decoder = decode_instruction_jump},
    {.id = JZ, .name = "jz", .decoder = decode_instruction_jump},
};

struct two_byte_instruction instructions_two_byte_prefix[] = {
    {
        .id = MOV,
        .name = "mov",
    },
    {
        .id = ADD,
        .name = "add",
    },
    {
        .id = SUB,
        .name = "sub",
    },
    {
        .id = CMP,
        .name = "cmp",
    },

};

uint16_t (*operations[])(uint16_t *destination, uint16_t *source) = {mov, add,
                                                                     sub, cmp};

uint16_t jump_masks[] = {
    // JNZ
    ZERO,
};

uint16_t jump_values[] = {
    // JNZ
    0,
};