// main takes one argument and that is a path to binary file of 8086 executable.
// It prints out assembly instructions in a format that can be again fed to nasm
// assembler. Also it simulates the given instructions and prints vaules of
// registers at the end of the disassembled output.

#include "simulate_8086_ISA.h"
#include "debug_output.c"
#include "tables.c"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define INSTRUCTION_MODE_REGISTER_TO_MEMORY_NO_DISPLACEMENT 0
#define INSTRUCTION_MODE_REGISTER_TO_MEMORY_BYTE_DISPLACEMENT 1
#define INSTRUCTION_MODE_REGISTER_TO_MEMORY_TWO_BYTE_DISPLACEMENT 2
#define INSTRUCTION_MODE_REGISTER 3
#define UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH 18

// read one or two bytes based on n_bytes variable
int read_n_bytes_as_number(size_t n_bytes, uint16_t *number_output,
                           FILE *executable) {
  if (n_bytes != 2 && n_bytes != 1) {
    fprintf(stderr, "expected 1 or 2 bytes to read, got %lu\n", n_bytes);
    return -1;
  }
  uint8_t buffer[2] = {0};
  if (fread(buffer, n_bytes, 1, executable) != 1) {
    fprintf(stderr, "failed to read %lu bytes\n", n_bytes);
    return -1;
  }

  *number_output = buffer[0];
  *number_output = (*number_output) | ((uint16_t)(buffer[1]) << 8);
  debug_byte_as_binary("read_n_bytes_as_number: buffer[0]", buffer[0]);
  debug_byte_as_binary("read_n_bytes_as_number: buffer[1]", buffer[1]);
  fprintf(stderr, "read_n_bytes_as_number, read number: %u\n", *number_output);
  return 1;
}

int decode_instruction_immediate_to_reg(struct context *ctx,
                                        uint8_t instruction_byte,
                                        FILE *executable,
                                        const char *instruction_name) {
  fprintf(stderr, "this is %s immediate to reg\n", instruction_name);
  const uint8_t word_mask = 0x08;
  const uint8_t register_mask = 0x07;
  uint8_t word = (instruction_byte & word_mask) >> 3;
  uint8_t reg = instruction_byte & register_mask;

  uint16_t number = 0;
  size_t size = (word + 1);
  int n = read_n_bytes_as_number(size, &number, executable);
  if (n < 0) {
    return EXIT_FAILURE;
  }

  // print
  printf("%s %s, %u\n", instruction_name, register_word_map[word][reg], number);

  // simulate
  uint8_t register_index = reg;

  if (word == 0) {
    if (reg > 3) {
      register_index = reg - 4;
      number = number << 8;
    }
    ctx->registers[register_index] = ctx->registers[register_index] | number;
  } else {
    ctx->registers[register_index] = number;
  }

  return EXIT_SUCCESS;
}

int decode_instruction_immediate_to_accumulator(struct context *ctx,
                                                uint8_t instruction_byte,
                                                FILE *executable,
                                                const char *instruction_name) {
  fprintf(stderr, "this is %s immediate to accumulator\n", instruction_name);
  const uint8_t word_mask = 0x01;
  uint8_t word = instruction_byte & word_mask;

  uint16_t number = 0;
  size_t size = (word + 1);
  int n = read_n_bytes_as_number(size, &number, executable);
  if (n < 0) {
    return EXIT_FAILURE;
  }

  printf("%s %s, %u\n", instruction_name, accumulator_registers[word], number);

  return EXIT_SUCCESS;
}

int decode_instruction_jump(struct context *ctx, uint8_t instruction_byte,
                            FILE *executable, const char *instruction_name) {
  fprintf(stderr, "this is %s\n", instruction_name);

  int8_t number = 0;

  if (fread(&number, sizeof(int8_t), 1, executable) != 1) {
    fprintf(stderr, "failed to read next byte for %s instruction\n",
            instruction_name);
    return EXIT_FAILURE;
  }

  printf("%s %d\n", instruction_name, number);

  return EXIT_SUCCESS;
}
int load_n_bytes_displacement(uint8_t n_bytes_displacement,
                              char formated_displacement_output[],
                              uint8_t formated_displacement_output_legth,
                              const char *displacement_template,
                              FILE *executable) {
  if (n_bytes_displacement != 2 && n_bytes_displacement != 1) {
    fprintf(stderr, "expected 1 or 2 bytes displacement, got %u\n",
            n_bytes_displacement);
    return -1;
  }

  uint16_t displacement = 0;
  int n =
      read_n_bytes_as_number(n_bytes_displacement, &displacement, executable);
  if (n < 0) {
    return -1;
  }

  n = snprintf(formated_displacement_output, formated_displacement_output_legth,
               displacement_template, displacement);

  if (n < 0 || n >= UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH) {
    fprintf(stderr, "failed to format displacement, n is %d\n", n);
    return -1;
  }
  return 1;
}

int decode_instruction_reg_mem_reg(struct context *ctx,
                                   uint8_t instruction_byte, FILE *executable,
                                   const char *instruction_name) {
  const uint8_t direction_mask = 0x02;
  const uint8_t word_mask = 0x01;
  const uint8_t mode_mask = 0xc0;
  const uint8_t register_mask = 0x38;
  const uint8_t register_memory_mask = 0x07;

  fprintf(stderr, "it's %s type 'Register/memory to/from register' \n",
          instruction_name);
  uint8_t direction = (instruction_byte & direction_mask) >> 1;
  uint8_t word = instruction_byte & word_mask;
  fprintf(stderr, "direction is %u\n", direction);
  fprintf(stderr, "word is %u\n", word);

  size_t n = fread(&instruction_byte, sizeof(instruction_byte), 1, executable);
  if (n != 1) {
    fprintf(stderr,
            "expected one byte for %s instruction to be complete, got none\n",
            instruction_name);
    return EXIT_FAILURE;
  }
  debug_byte_as_binary("next byte as binary: ", instruction_byte);

  uint8_t mode = (instruction_byte & mode_mask) >> 6;
  uint8_t reg = (instruction_byte & register_mask) >> 3;
  uint8_t reg_mem = instruction_byte & register_memory_mask;
  debug_byte_as_binary("reg:", reg);
  debug_byte_as_binary("reg/mem:", reg_mem);

  const char *destination = register_word_map[word][reg];
  const char *source = NULL;

  char displacement_formated_buffer
      [UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH] = {0};

  switch (mode) {
  case INSTRUCTION_MODE_REGISTER:
    fprintf(stderr, "%s mode: Register Mode\n", instruction_name);
    source = register_word_map[word][reg_mem];
    break;
  case INSTRUCTION_MODE_REGISTER_TO_MEMORY_NO_DISPLACEMENT:
    fprintf(stderr, "%s mode: Memory Mode, no displacement\n",
            instruction_name);
    source = memory_displacement_expresion_table[mode][reg_mem];
    // TODO handle specail case for MODE=0b110 - direct address.
    break;
  case INSTRUCTION_MODE_REGISTER_TO_MEMORY_BYTE_DISPLACEMENT:
    // Let's FALLTHROUGH as this can be handled by following case by utilizing
    // mode value itself.
  case INSTRUCTION_MODE_REGISTER_TO_MEMORY_TWO_BYTE_DISPLACEMENT:
    fprintf(stderr, "%s mode: Memory Mode, %u byte displacement\n",
            instruction_name, mode);
    source = memory_displacement_expresion_table[mode][reg_mem];
    int status = load_n_bytes_displacement(
        mode, displacement_formated_buffer,
        UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH, source, executable);
    if (status < 0) {
      return EXIT_FAILURE;
    }
    fprintf(stderr, "displacement: %s\n", displacement_formated_buffer);
    source = displacement_formated_buffer;
    break;
  default:
    debug_byte_as_binary("unknown mode: ", mode);
    return EXIT_FAILURE;
  }
  if (direction == 0) {
    // reverse default destination, source order of arguments in the instruction
    const char *tmp = destination;
    destination = source;
    source = tmp;
  }
  printf("%s %s, %s\n", instruction_name, destination, source);
  return EXIT_SUCCESS;
}

int decode_instruction_immediate_to_memory_reg(struct context *ctx,
                                               uint16_t instruction_word,
                                               FILE *executable,
                                               const char *instruction_name) {
  const uint16_t word_mask = 0x0100;
  const uint16_t sign_extension_mask = 0x0200;
  const uint16_t mode_mask = 0x00c0;
  const uint16_t register_memory_mask = 0x0007;

  fprintf(stderr, "it's %s type 'Immediate to register/memory' \n",
          instruction_name);

  uint8_t word = (instruction_word & word_mask) >> 8;
  uint8_t sign_extension = (instruction_word & sign_extension_mask) >> 9;
  fprintf(stderr, "word is %u\n", word);
  fprintf(stderr, "sign_extension is %u\n", sign_extension);

  uint8_t mode = (instruction_word & mode_mask) >> 6;
  uint8_t reg_mem = instruction_word & register_memory_mask;
  debug_byte_as_binary("reg/mem:", reg_mem);

  const char *destination = NULL;
  const char *size_modifier = "";
  uint16_t number = 0;

  char displacement_formated_buffer
      [UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH] = {0};

  switch (mode) {
  case INSTRUCTION_MODE_REGISTER:
    fprintf(stderr, "%s mode: Register Mode\n", instruction_name);
    destination = register_word_map[word][reg_mem];
    break;
  case INSTRUCTION_MODE_REGISTER_TO_MEMORY_NO_DISPLACEMENT:
    fprintf(stderr, "%s mode: Memory Mode, no displacement\n",
            instruction_name);
    destination = memory_displacement_expresion_table[mode][reg_mem];
    size_modifier = size_modifiers[word];
    if (reg_mem == 6) {
      // handle specail case for MODE=0b110 - direct address.
      fprintf(stderr,
              "%s mode: Memory Mode, special mode 6 - 16bit displacement\n",
              instruction_name);
      int n = load_n_bytes_displacement(
          2, displacement_formated_buffer,
          UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH, destination,
          executable);
      if (n < 0) {
        return EXIT_FAILURE;
      }
      destination = displacement_formated_buffer;
    }
    break;
  case INSTRUCTION_MODE_REGISTER_TO_MEMORY_BYTE_DISPLACEMENT:
    // Let's FALLTHROUGH as this can be handled by following case by utilizing
    // mode value itself.
  case INSTRUCTION_MODE_REGISTER_TO_MEMORY_TWO_BYTE_DISPLACEMENT:
    fprintf(stderr, "%s mode: Memory Mode, %u byte displacement\n",
            instruction_name, mode);
    destination = memory_displacement_expresion_table[mode][reg_mem];
    int n = load_n_bytes_displacement(
        mode, displacement_formated_buffer,
        UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH, destination,
        executable);
    if (n < 0) {
      return EXIT_FAILURE;
    }
    fprintf(stderr, "displacement: %s\n", displacement_formated_buffer);
    destination = displacement_formated_buffer;
    size_modifier = size_modifiers[word];
    break;
  default:
    debug_byte_as_binary("unknown mode: ", mode);
    return EXIT_FAILURE;
  }
  size_t size;
  if (sign_extension == 0 && word == 1) {
    size = 2;
  } else {
    size = 1;
  }
  int n = read_n_bytes_as_number(size, &number, executable);
  if (n < 0) {
    return EXIT_FAILURE;
  }
  printf("%s %s %s, %u\n", instruction_name, size_modifier, destination,
         number);
  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  if (argc > 3 || argc < 2) {
    fprintf(
        stderr,
        "%s expects optional '-exec' flag and mandatory 8086 executable path "
        "%d arguments were given\n",
        argv[0], argc - 1);
    return EXIT_FAILURE;
  }
  const char *filename;
  int exec = 0;
  if (argc == 2) {
    filename = argv[1];
  } else {
    filename = argv[2];
    exec = 1;
  }
  fprintf(stderr, "disassembling file %s\n", filename);
  FILE *executable = fopen(filename, "rb");
  uint8_t instruction_byte;

  uint64_t single_byte_decoders_count =
      sizeof(single_byte_instruction_prefix_decoders) /
      sizeof(single_byte_instruction_prefix_decoders[0]);

  uint64_t two_byte_decoders_count =
      sizeof(two_byte_instruction_prefix_decoders) /
      sizeof(two_byte_instruction_prefix_decoders[0]);

  int exit_code = EXIT_SUCCESS;
  struct context ctx = {.memory = {0}, .registers = {0}};
  printf("bits 16\n\n");
  while (fread(&instruction_byte, sizeof(instruction_byte), 1, executable) ==
         1) {

    debug_byte_as_binary("first instruction byte as binary:", instruction_byte);
    int found = 0;
    for (uint64_t i = 0; i < single_byte_decoders_count; i++) {
      uint8_t opcode_mask = instruction_opcode_single_byte_prefixes[i].mask;
      uint8_t opcode_prefix = instruction_opcode_single_byte_prefixes[i].value;
      debug_byte_as_binary("trying following prefix:", opcode_prefix);
      if ((instruction_byte & opcode_mask) == opcode_prefix) {
        found = 1;
        exit_code = single_byte_instruction_prefix_decoders[i](
            &ctx, instruction_byte, executable,
            instruction_names_single_byte_prefix[i]);
        if (exit_code != EXIT_SUCCESS) {
          goto exit;
        }
        break;
      }
    }
    if (found == 1) {
      continue;
    }
    uint16_t instruction_word = 0;
    instruction_word = instruction_word | ((uint16_t)(instruction_byte) << 8);
    // Load next byte so we can decide on two byte prefix when one byte prefix
    // failed.
    size_t n =
        fread(&instruction_byte, sizeof(instruction_byte), 1, executable);
    if (n != 1) {
      fprintf(stderr,
              "failed to load next byte befor testing for two byte "
              "instructions prefixes, n is %lu",
              n);
      return EXIT_FAILURE;
    }
    instruction_word = instruction_word | ((uint16_t)instruction_byte);
    debug_uint16_as_binary("first two instruction bytes as binary: ",
                           instruction_word);
    for (uint64_t i = 0; i < two_byte_decoders_count; i++) {
      uint16_t opcode_mask = instruction_opcode_two_byte_prefixes[i].mask;
      uint16_t opcode_prefix = instruction_opcode_two_byte_prefixes[i].value;
      debug_uint16_as_binary("trying following prefix: ", opcode_prefix);
      if ((instruction_word & opcode_mask) == opcode_prefix) {
        found = 1;
        // TODO, do we even need this table when it contains only one decoder??
        // It will most likely change as each jump may have different handler
        // for simulation.
        exit_code = two_byte_instruction_prefix_decoders[i](
            &ctx, instruction_word, executable,
            instruction_names_two_byte_prefix[i]);
        if (exit_code != EXIT_SUCCESS) {
          goto exit;
        }
        break;
      }
    }

    if (found == 0) {
      fprintf(stderr, "no decoder exists for this instruction\n");
      exit_code = EXIT_FAILURE;
      goto exit;
    }

    fprintf(stderr, "---- next instruction ----\n");
  }

  if (exec == 1) {
    // TODO actually don't execute the instructions instead of just limiting the
    // output.
    printf("Final registers:\n");
    for (int i = 0; i < REGISTER_COUNT; ++i) {
      printf("\t%s: %x (%d)\n", register_word_map[1][i], ctx.registers[i],
             ctx.registers[i]);
    }
  }

exit:
  fclose(executable);
  return exit_code;
}