#ifndef SIMULATE_HEADER
#define SIMULATE_HEADER 1

#include <stdint.h>
#include <stdio.h>

struct single_byte_prefix_instruction {
  uint8_t mask;
  uint8_t value;
};

struct two_byte_prefix_instruction {
  uint16_t mask;
  uint16_t value;
};

struct context {
  int16_t registers[8];
  int8_t memory[1024 * 1024];
};

int decode_instruction_immediate_to_reg(struct context *ctx,
                                        uint8_t instruction_byte,
                                        FILE *executable,
                                        const char *instruction_name);

int decode_instruction_immediate_to_accumulator(struct context *ctx,
                                                uint8_t instruction_byte,
                                                FILE *executable,
                                                const char *instruction_name);

int decode_instruction_reg_mem_reg(struct context *ctx,
                                   uint8_t instruction_byte,
                                   FILE *executable,
                                   const char *instruction_name);

int decode_instruction_immediate_to_memory_reg(struct context *ctx,
                                               uint16_t instruction_word,
                                               FILE *executable,
                                               const char *instruction_name);

int decode_instruction_jump(struct context *ctx, uint8_t instruction_byte,
                            FILE *executable, const char *instruction_name);
#endif