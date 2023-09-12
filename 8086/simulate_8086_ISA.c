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
#include <string.h>
#include <sys/types.h>

#define INSTRUCTION_MODE_REGISTER_TO_MEMORY_NO_DISPLACEMENT 0
#define INSTRUCTION_MODE_REGISTER_TO_MEMORY_BYTE_DISPLACEMENT 1
#define INSTRUCTION_MODE_REGISTER_TO_MEMORY_TWO_BYTE_DISPLACEMENT 2
#define INSTRUCTION_MODE_REGISTER 3
#define UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH 18

// read one or two bytes based on n_bytes variable
int read_n_bytes_as_number(struct context *ctx, size_t n_bytes,
                           uint16_t *number_output,
                           uint8_t instruction_buffer[]) {
  if (n_bytes != 2 && n_bytes != 1) {
    fprintf(stderr, "expected 1 or 2 bytes to read, got %lu\n", n_bytes);
    return -1;
  }
  uint8_t buffer[2] = {0};
  memcpy(buffer, &(instruction_buffer[ctx->ip]), n_bytes);

  *number_output = buffer[0];
  *number_output = (*number_output) | ((uint16_t)(buffer[1]) << 8);
  debug_byte_as_binary("read_n_bytes_as_number: buffer[0]", buffer[0]);
  debug_byte_as_binary("read_n_bytes_as_number: buffer[1]", buffer[1]);
  fprintf(stderr, "read_n_bytes_as_number, read number: %u\n", *number_output);
  return 1;
}

int decode_instruction_immediate_to_reg(struct context *ctx,
                                        uint8_t instruction_byte,
                                        uint8_t instruction_buffer[],
                                        struct instruction instruction) {
  fprintf(stderr, "this is %s immediate to reg\n", instruction.name);
  const uint8_t word_mask = 0x08;
  const uint8_t register_mask = 0x07;
  uint8_t word = (instruction_byte & word_mask) >> 3;
  uint8_t reg = instruction_byte & register_mask;

  uint16_t number = 0;
  size_t size = (word + 1);
  ctx->ip += 1;
  int n = read_n_bytes_as_number(ctx, size, &number, instruction_buffer);
  if (n < 0) {
    return EXIT_FAILURE;
  }
  ctx->ip += size;

  // print
  printf("%s %s, %u\n", instruction.name, register_word_map[word][reg], number);

  // simulate
  uint8_t register_index = reg;

  if (word == 1) {
    // Let's simulate only whole registers for now.
    ctx->flags =
        operations[instruction.id](&(ctx->registers[register_index]), &number);
  }

  return EXIT_SUCCESS;
}

int decode_instruction_immediate_to_accumulator(
    struct context *ctx, uint8_t instruction_byte, uint8_t instruction_buffer[],
    struct instruction instruction) {
  fprintf(stderr, "this is %s immediate to accumulator\n", instruction.name);
  const uint8_t word_mask = 0x01;
  uint8_t word = instruction_byte & word_mask;

  uint16_t number = 0;
  size_t size = (word + 1);
  ctx->ip += 1;
  int n = read_n_bytes_as_number(ctx, size, &number, instruction_buffer);
  if (n < 0) {
    return EXIT_FAILURE;
  }
  ctx->ip += size;

  printf("%s %s, %u\n", instruction.name, accumulator_registers[word], number);

  return EXIT_SUCCESS;
}

int decode_instruction_jump(struct context *ctx, uint8_t instruction_byte,
                            uint8_t instruction_buffer[],
                            struct instruction instruction) {
  fprintf(stderr, "this is %s\n", instruction.name);

  int8_t number = 0;
  ctx->ip += 1;
  number = instruction_buffer[ctx->ip];
  ctx->ip += 1;

  fprintf(stderr, "ip before simulation: %u\n", ctx->ip);

  // note: we need to substract 4 because there are other 4 non-jump
  // instructions in the enum
  // TODO: maybe we could move jump instructions in separate enum so we don't
  // have to do the substraction.
  if ((ctx->flags & jump_masks[instruction.id - 4]) ==
      jump_values[instruction.id - 4]) {
    ctx->ip = (int16_t)(ctx->ip) + number;
  }

  fprintf(stderr, "ip after simulation: %u\n", ctx->ip);

  printf("%s %d\n", instruction.name, number);

  return EXIT_SUCCESS;
}

int load_n_bytes_displacement(struct context *ctx, uint8_t n_bytes_displacement,
                              char formated_displacement_output[],
                              uint8_t formated_displacement_output_legth,
                              struct displacement displacement,
                              uint8_t instruction_buffer[],
                              uint16_t *memory_address) {
  if (n_bytes_displacement > 2) {
    fprintf(stderr, "expected 0, 1 or 2 bytes displacement, got %u\n",
            n_bytes_displacement);
    return -1;
  }

  uint16_t offset = 0;
  int n = read_n_bytes_as_number(ctx, n_bytes_displacement, &offset,
                                 instruction_buffer);
  if (n < 0) {
    return -1;
  }

  n = snprintf(formated_displacement_output, formated_displacement_output_legth,
               displacement.template, offset);

  if (n < 0 || n >= UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH) {
    fprintf(stderr, "failed to format displacement, n is %d\n", n);
    return -1;
  }

  // Add registers to the offset if there are any.
  offset += ctx->registers[displacement.first_register];
  offset += ctx->registers[displacement.second_register];

  // Return final offset using this pointer.
  *memory_address = offset;

  return 0;
}

int decode_instruction_reg_mem_reg(struct context *ctx,
                                   uint8_t instruction_byte,
                                   uint8_t instruction_buffer[],
                                   struct instruction instruction) {
  const uint8_t direction_mask = 0x02;
  const uint8_t word_mask = 0x01;
  const uint8_t mode_mask = 0xc0;
  const uint8_t register_mask = 0x38;
  const uint8_t register_memory_mask = 0x07;

  fprintf(stderr, "it's %s type 'Register/memory to/from register' \n",
          instruction.name);
  uint8_t direction = (instruction_byte & direction_mask) >> 1;
  uint8_t word = instruction_byte & word_mask;
  fprintf(stderr, "direction is %u\n", direction);
  fprintf(stderr, "word is %u\n", word);

  ctx->ip += 1;
  instruction_byte = instruction_buffer[ctx->ip];
  debug_byte_as_binary("next byte as binary: ", instruction_byte);
  ctx->ip += 1;

  uint8_t mode = (instruction_byte & mode_mask) >> 6;
  uint8_t reg = (instruction_byte & register_mask) >> 3;
  uint8_t reg_mem = instruction_byte & register_memory_mask;
  debug_byte_as_binary("reg:", reg);
  debug_byte_as_binary("reg/mem:", reg_mem);

  const char *destination = register_word_map[word][reg];
  const char *source = NULL;

  char displacement_formated_buffer
      [UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH] = {0};

  uint16_t *exec_destination = NULL;
  exec_destination = &(ctx->registers[reg]);
  uint16_t *exec_source = NULL;
  uint16_t memory_address = 0;
  struct displacement displacement;

  switch (mode) {
  case INSTRUCTION_MODE_REGISTER:
    fprintf(stderr, "%s mode: Register Mode\n", instruction.name);
    source = register_word_map[word][reg_mem];
    // simulate
    if (word == 1) {
      // let's simulate only whole registers for now.
      exec_source = &(ctx->registers[reg_mem]);
    }

    break;
  case INSTRUCTION_MODE_REGISTER_TO_MEMORY_NO_DISPLACEMENT:
    fprintf(stderr, "%s mode: Memory Mode, no displacement\n",
            instruction.name);
    displacement = memory_displacements[mode][reg_mem];
    uint8_t displacement_size;
    if (reg_mem == 6) {
      // handle specail case for MODE=0b110 - direct address.
      fprintf(stderr,
              "%s mode: Memory Mode, special mode 6 - 16bit displacement\n",
              instruction.name);
      displacement_size = 2;
    } else {
      displacement_size = 0;
    }
    int n = load_n_bytes_displacement(
        ctx, displacement_size, displacement_formated_buffer,
        UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH, displacement,
        instruction_buffer, &memory_address);
    if (n < 0) {
      return EXIT_FAILURE;
    }
    ctx->ip += displacement_size;
    source = displacement_formated_buffer;
    exec_source = &(ctx->memory[memory_address]);
    break;
  case INSTRUCTION_MODE_REGISTER_TO_MEMORY_BYTE_DISPLACEMENT:
    // Let's FALLTHROUGH as this can be handled by following case by utilizing
    // mode value itself.
  case INSTRUCTION_MODE_REGISTER_TO_MEMORY_TWO_BYTE_DISPLACEMENT:
    fprintf(stderr, "%s mode: Memory Mode, %u byte displacement\n",
            instruction.name, mode);
    displacement = memory_displacements[mode][reg_mem];
    int status = load_n_bytes_displacement(
        ctx, mode, displacement_formated_buffer,
        UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH, displacement,
        instruction_buffer, &memory_address);
    if (status < 0) {
      return EXIT_FAILURE;
    }
    fprintf(stderr, "displacement: %s\n", displacement_formated_buffer);
    source = displacement_formated_buffer;
    ctx->ip += mode;
    exec_source = &(ctx->memory[memory_address]);
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

    uint16_t *temp = exec_destination;
    exec_destination = exec_source;
    exec_source = temp;
  }

  // simulate
  if (exec_destination != NULL && exec_source != NULL && word == 1) {
    // Let's simulate only whole registers for now.
    ctx->flags = operations[instruction.id](exec_destination, exec_source);
  }

  printf("%s %s, %s\n", instruction.name, destination, source);
  return EXIT_SUCCESS;
}

int decode_instruction_immediate_to_memory_reg(
    struct context *ctx, uint16_t instruction_word,
    uint8_t instruction_buffer[], struct two_byte_instruction instruction) {
  const uint16_t word_mask = 0x0100;
  const uint16_t sign_extension_mask = 0x0200;
  const uint16_t mode_mask = 0x00c0;
  const uint16_t register_memory_mask = 0x0007;

  fprintf(stderr, "it's %s type 'Immediate to register/memory' \n",
          instruction.name);

  uint8_t word = (instruction_word & word_mask) >> 8;
  uint8_t sign_extension;
  if (instruction.id == MOV) {
    // mov has this bit set to 1 always and size is guided only by word bit.
    sign_extension = 0;
  } else {
    sign_extension = (instruction_word & sign_extension_mask) >> 9;
  }
  fprintf(stderr, "word is %u\n", word);
  fprintf(stderr, "sign_extension is %u\n", sign_extension);

  uint8_t mode = (instruction_word & mode_mask) >> 6;
  uint8_t reg_mem = instruction_word & register_memory_mask;
  debug_byte_as_binary("reg/mem:", reg_mem);

  struct displacement displacement;
  const char *destination = NULL;
  const char *size_modifier = "";
  uint16_t number = 0;
  uint16_t memory_address = 0;

  char displacement_formated_buffer
      [UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH] = {0};

  // exec_destination is a pointer pointing to an address where the immediate
  // should be stored.
  uint16_t *exec_destination = NULL;

  ctx->ip += 2;
  switch (mode) {
  case INSTRUCTION_MODE_REGISTER:
    fprintf(stderr, "%s mode: Register Mode\n", instruction.name);
    destination = register_word_map[word][reg_mem];
    exec_destination = &(ctx->registers[reg_mem]);
    break;
  case INSTRUCTION_MODE_REGISTER_TO_MEMORY_NO_DISPLACEMENT:
    fprintf(stderr, "%s mode: Memory Mode, no displacement\n",
            instruction.name);
    size_modifier = size_modifiers[word];
    displacement = memory_displacements[mode][reg_mem];
    uint8_t displacement_size;
    if (reg_mem == 6) {
      // handle specail case for MODE=0b110 - direct address.
      fprintf(stderr,
              "%s mode: Memory Mode, special mode 6 - 16bit displacement\n",
              instruction.name);
      displacement_size = 2;
    } else {
      displacement_size = 0;
    }
    int n = load_n_bytes_displacement(
        ctx, displacement_size, displacement_formated_buffer,
        UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH, displacement,
        instruction_buffer, &memory_address);
    if (n < 0) {
      return EXIT_FAILURE;
    }
    ctx->ip += displacement_size;
    destination = displacement_formated_buffer;
    exec_destination = &(ctx->memory[memory_address]);
    fprintf(stderr, "exec_destination: %p\n", exec_destination);
    break;
  case INSTRUCTION_MODE_REGISTER_TO_MEMORY_BYTE_DISPLACEMENT:
    // Let's FALLTHROUGH as this can be handled by following case by utilizing
    // mode value itself.
  case INSTRUCTION_MODE_REGISTER_TO_MEMORY_TWO_BYTE_DISPLACEMENT:
    fprintf(stderr, "%s mode: Memory Mode, %u byte displacement\n",
            instruction.name, mode);
    displacement = memory_displacements[mode][reg_mem];
    n = load_n_bytes_displacement(
        ctx, mode, displacement_formated_buffer,
        UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH, displacement,
        instruction_buffer, &memory_address);
    if (n < 0) {
      return EXIT_FAILURE;
    }
    fprintf(stderr, "displacement: %s\n", displacement_formated_buffer);
    destination = displacement_formated_buffer;
    size_modifier = size_modifiers[word];
    exec_destination = &(ctx->memory[memory_address]);
    ctx->ip += mode;
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
  int n = read_n_bytes_as_number(ctx, size, &number, instruction_buffer);
  if (n < 0) {
    return EXIT_FAILURE;
  }
  ctx->ip += size;

  // simulate
  if (exec_destination != NULL) {
    // Let's simulate only whole registers for now.
    ctx->flags = operations[instruction.id](exec_destination, &number);
  }

  printf("%s %s %s, %u\n", instruction.name, size_modifier, destination,
         number);
  return EXIT_SUCCESS;
}

uint16_t mov(uint16_t *destination, uint16_t *source) {
  *destination = *source;
  return 0;
}
uint16_t add(uint16_t *destination, uint16_t *source) {
  uint16_t result = *destination + *source;
  *destination = result;
  uint16_t flags = 0;
  if (result == 0) {
    flags = flags | ZERO;
  } else if ((0x8000 & result) != 0) {
    flags = flags | SIGN;
  }

  return flags;
}
void print_flags(uint16_t flags) {
  printf("\tflags: ");
  if ((flags & ZERO) != 0) {
    printf("Z");
  }
  if ((flags & SIGN) != 0) {
    printf("S");
  }
  printf("\n");
}
uint16_t sub(uint16_t *destination, uint16_t *source) {
  uint16_t result = *destination - *source;
  *destination = result;
  uint16_t flags = 0;
  if (result == 0) {
    flags = flags | ZERO;
  } else if ((0x8000 & result) != 0) {
    flags = flags | SIGN;
  }
  return flags;
}
uint16_t cmp(uint16_t *destination, uint16_t *source) {
  uint16_t result = *destination - *source;
  uint16_t flags = 0;
  if (result == 0) {
    flags = flags | ZERO;
  } else if ((0x8000 & result) != 0) {
    flags = flags | SIGN;
  }
  print_flags(flags);
  return flags;
}

int main(int argc, char *argv[]) {
  if (argc > 4 || argc < 2) {
    fprintf(stderr,
            "%s expects optional '-exec' and '-dump' flags and mandatory 8086 "
            "executable path "
            "%d arguments were given\n",
            argv[0], argc - 1);
    return EXIT_FAILURE;
  }
  const char *filename;
  int exec = 0;
  int dump = 0;
  switch (argc) {
  case 2:
    filename = argv[1];
    break;
  case 3:
    filename = argv[2];
    if (strcmp(argv[1], "-exec") == 0) {
      exec = 1;
    } else if (strcmp(argv[1], "-dump") == 0) {
      dump = 1;
    } else {
      fprintf(stderr, "no such flag '%s'\n", argv[1]);
      return EXIT_FAILURE;
    }
    break;
  case 4:
    filename = argv[3];
    if (strcmp(argv[1], "-exec") == 0 && strcmp(argv[2], "-dump") == 0) {
      exec = 1;
      dump = 1;
    } else if (strcmp(argv[2], "-exec") == 0 && strcmp(argv[1], "-dump") == 0) {
      exec = 1;
      dump = 1;
    } else {
      fprintf(stderr, "one of the flags does not exist '%s', '%s'\n", argv[1],
              argv[2]);
      return EXIT_FAILURE;
    }
    break;
  default:
    fprintf(stderr, "invalid number of arguments\n");
    return EXIT_FAILURE;
  }
  fprintf(stderr, "disassembling file %s, exec is %d, dump is %d\n", filename,
          exec, dump);
  FILE *executable = fopen(filename, "rb");
  fseek(executable, 0, SEEK_END);
  long fsize = ftell(executable);
  fseek(executable, 0, SEEK_SET);

  uint8_t *instructions_buffer = malloc(fsize);
  size_t n = fread(instructions_buffer, fsize, 1, executable);
  fclose(executable);
  if (n != 1) {
    fprintf(stderr,
            "failed to read given exectuable into memory, n=%lu bytes read\n",
            n);
    return EXIT_FAILURE;
  }

  uint8_t instruction_byte;

  uint64_t single_byte_prefix_instructions_count =
      sizeof(instructions_single_byte_prefix) /
      sizeof(instructions_single_byte_prefix[0]);

  uint64_t two_byte_instructions_count =
      sizeof(instructions_two_byte_prefix) /
      sizeof(instructions_two_byte_prefix[0]);

  int exit_code = EXIT_SUCCESS;
  struct context ctx = {.memory = {0}, .registers = {0}, .ip = 0};

  printf("bits 16\n\n");

  // TODO implement binary search or some other smarter way to lookup the
  // prefix.
  while (ctx.ip < fsize) {
    fprintf(stderr, "---- next instruction ----\n");
    instruction_byte = instructions_buffer[ctx.ip];
    debug_byte_as_binary("first instruction byte as binary:", instruction_byte);
    int found = 0;
    for (uint64_t i = 0; i < single_byte_prefix_instructions_count; i++) {
      struct instruction instruction = instructions_single_byte_prefix[i];
      uint8_t opcode_mask = instruction_opcode_single_byte_prefixes[i].mask;
      uint8_t opcode_prefix = instruction_opcode_single_byte_prefixes[i].value;
      debug_byte_as_binary("trying following prefix:", opcode_prefix);
      if ((instruction_byte & opcode_mask) == opcode_prefix) {
        found = 1;
        exit_code = instructions_single_byte_prefix[i].decoder(
            &ctx, instruction_byte, instructions_buffer,
            instructions_single_byte_prefix[i]);
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
    instruction_byte = instructions_buffer[ctx.ip + 1];
    instruction_word = instruction_word | ((uint16_t)instruction_byte);
    debug_uint16_as_binary("first two instruction bytes as binary: ",
                           instruction_word);
    for (uint64_t i = 0; i < two_byte_instructions_count; i++) {
      uint16_t opcode_mask = instruction_opcode_two_byte_prefixes[i].mask;
      uint16_t opcode_prefix = instruction_opcode_two_byte_prefixes[i].value;
      debug_uint16_as_binary("trying following prefix: ", opcode_prefix);
      if ((instruction_word & opcode_mask) == opcode_prefix) {
        found = 1;
        // Note: there is only single instruction type where we need to check
        // first two bytes. Thus there is only one decoder.
        exit_code = decode_instruction_immediate_to_memory_reg(
            &ctx, instruction_word, instructions_buffer,
            instructions_two_byte_prefix[i]);
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
  }

  if (exec == 1) {
    printf("Final registers:\n");
    for (int i = 0; i < REGISTER_COUNT; ++i) {
      printf("\t%s: %04x (%d)\n", register_word_map[1][i], ctx.registers[i],
             ctx.registers[i]);
    }
    printf("\tip: %04x (%d)\n", ctx.ip, ctx.ip);
    print_flags(ctx.flags);
  }

  if (dump == 1) {
    char *dump_data_filename = malloc(sizeof(char) * strlen(filename));
    memcpy(dump_data_filename, filename, sizeof(char) * strlen(filename));
    dump_data_filename = strcat(dump_data_filename, ".data");
    fprintf(stderr, "dumping the memory to a file %s\n", dump_data_filename);
    FILE *data = fopen(dump_data_filename, "wb");
    if (data == NULL) {
      fprintf(stderr, "failed to open the file");
    }
    free(dump_data_filename);
    size_t memory_size = sizeof(ctx.memory) / sizeof(ctx.memory[0]);
    int n = fwrite(ctx.memory, memory_size, 1, data);
    if (n != 1) {
      exit_code = EXIT_FAILURE;
      fprintf(stderr, "failed to write memory to a file\n");
    }
    fclose(data);
  }
exit:
  free(instructions_buffer);
  return exit_code;
}
