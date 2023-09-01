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
  uint16_t registers[8];
  uint8_t memory[1024 * 1024];
};

enum instruction {
  MOV,
  ADD,
  SUB,
  CMP,
  JNZ,
  JL,
  JLE,
  JB,
  JBE,
  JP,
  JO,
  JS,
  JNL,
  JG,
  JNB,
  JA,
  JNP,
  JNO,
  JNS,
  LOOP,
  LOOPZ,
  LOOPNZ,
  JCXZ,
  JZ,
};

uint16_t mov(uint16_t *destination, uint16_t *source);
uint16_t add(uint16_t *destination, uint16_t *source);
uint16_t sub(uint16_t *destination, uint16_t *source);
uint16_t cmp(uint16_t *destination, uint16_t *source);

int decode_instruction_immediate_to_reg(struct context *ctx,
                                        uint8_t instruction_byte,
                                        FILE *executable,
                                        enum instruction instruction_id);

int decode_instruction_immediate_to_accumulator(
    struct context *ctx, uint8_t instruction_byte, FILE *executable,
    enum instruction instruction_id);

int decode_instruction_reg_mem_reg(struct context *ctx,
                                   uint8_t instruction_byte, FILE *executable,
                                   enum instruction instruction_id);

int decode_instruction_immediate_to_memory_reg(struct context *ctx,
                                               uint16_t instruction_word,
                                               FILE *executable,
                                               enum instruction instruction_id);

int decode_instruction_jump(struct context *ctx, uint8_t instruction_byte,
                            FILE *executable, enum instruction instruction_id);
#endif