// main takes one argument and that is a path to binary file of 8086 executable.
// It prints out assembly instructions in a format that can be again fed to nasm
// assembler.
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define INSTRUCTION_MODE_REGISTER_TO_MEMORY_NO_DISPLACEMENT 0
#define INSTRUCTION_MODE_REGISTER_TO_MEMORY_BYTE_DISPLACEMENT 1
#define INSTRUCTION_MODE_REGISTER_TO_MEMORY_TWO_BYTE_DISPLACEMENT 2
#define INSTRUCTION_MODE_REGISTER 3
#define UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH 18

void print_byte_as_binary(unsigned char input) {
  char output[(sizeof(input) * 8) + 1] = {0};
  int sizeof_input_bits = sizeof(input) * 8;
  int bit_position = sizeof_input_bits - 1;
  while (bit_position >= 0) {
    uint16_t mask = 1 << bit_position;
    if ((input & mask) > 0) {

      output[sizeof_input_bits - 1 - bit_position] = '1';
    } else {
      output[sizeof_input_bits - 1 - bit_position] = '0';
    }
    bit_position = bit_position - 1;
  }
  output[sizeof_input_bits] = '\0';
  fprintf(stderr, "%s", output);
  return;
}

void debug_byte_as_binary(const char *message, unsigned char byte) {
  fprintf(stderr, message);
  print_byte_as_binary(byte);
  fprintf(stderr, "\n");
}

static const char *register_word_map[][8] = {
    {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"},
    {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"},
};

// read one or two bytes based on n_bytes variable
int read_n_bytes_as_number(size_t n_bytes, uint16_t *number_output,
                           FILE *executable) {
  if (n_bytes != 2 && n_bytes != 1) {
    fprintf(stderr, "expected 1 or 2 bytes to read, got %u\n", n_bytes);
    return -1;
  }
  unsigned char buffer[2] = {0};
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

int decode_instruction_immediate_to_reg(unsigned char instruction_byte,
                                        FILE *executable,
                                        const char *instruction_name) {
  fprintf(stderr, "this is %s immediate to reg\n", instruction_name);
  const unsigned char word_mask = 0x08;
  const unsigned char register_mask = 0x07;
  unsigned char word = (instruction_byte & word_mask) >> 3;
  unsigned char reg = instruction_byte & register_mask;

  uint16_t number = 0;
  size_t size = (word + 1);
  int n = read_n_bytes_as_number(size, &number, executable);
  if (n < 0) {
    return EXIT_FAILURE;
  }

  printf("%s %s, %u\n", instruction_name, register_word_map[word][reg], number);

  return EXIT_SUCCESS;
}

static const char *memory_displacement_expresion_table[][8] = {
    {
        "[bx + si]",
        "[bx + di]",
        "[bp + si]",
        "[bp + di]",
        "[si]",
        "[di]",
        "%s",
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

  fprintf(stderr, "%s\n", formated_displacement_output);
  if (n < 0 || n >= UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH) {
    fprintf(stderr, "failed to format displacement, n is %lu\n", n);
    return -1;
  }
  return 1;
}

int decode_instruction_reg_mem_reg(unsigned char instruction_byte,
                                   FILE *executable,
                                   const char *instruction_name) {
  const unsigned char direction_mask = 0x02;
  const unsigned char word_mask = 0x01;
  const unsigned char mode_mask = 0xc0;
  const unsigned char register_mask = 0x38;
  const unsigned char register_memory_mask = 0x07;

  fprintf(stderr, "it's %s type 'Register/memory to/from register' \n",
          instruction_name);
  unsigned char direction = (instruction_byte & direction_mask) >> 1;
  unsigned char word = instruction_byte & word_mask;
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

  unsigned char mode = (instruction_byte & mode_mask) >> 6;
  unsigned char reg = (instruction_byte & register_mask) >> 3;
  unsigned char reg_mem = instruction_byte & register_memory_mask;
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
    n = load_n_bytes_displacement(
        mode, displacement_formated_buffer,
        UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH, source, executable);
    if (n < 0) {
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

int decode_instruction_immediate_to_memory_reg(unsigned char instruction_byte,
                                               FILE *executable,
                                               const char *instruction_name) {
  const unsigned char word_mask = 0x01;
  const unsigned char sign_extension_mask = 0x02;
  const unsigned char mode_mask = 0xc0;
  const unsigned char register_memory_mask = 0x07;

  fprintf(stderr, "it's %s type 'Immediate to register/memory' \n",
          instruction_name);

  unsigned char word = instruction_byte & word_mask;
  unsigned char sign_extension = (instruction_byte & sign_extension_mask) >> 1;
  fprintf(stderr, "word is %u\n", word);

  size_t n = fread(&instruction_byte, sizeof(instruction_byte), 1, executable);
  if (n != 1) {
    fprintf(stderr,
            "expected one byte for %s instruction to be complete, got none\n",
            instruction_name);
    return EXIT_FAILURE;
  }
  debug_byte_as_binary("next byte as binary: ", instruction_byte);

  unsigned char mode = (instruction_byte & mode_mask) >> 6;
  unsigned char reg_mem = instruction_byte & register_memory_mask;
  debug_byte_as_binary("reg/mem:", reg_mem);

  const char *destination = NULL;
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
    // TODO handle specail case for MODE=0b110 - direct address.
    break;
  case INSTRUCTION_MODE_REGISTER_TO_MEMORY_BYTE_DISPLACEMENT:
    // Let's FALLTHROUGH as this can be handled by following case by utilizing
    // mode value itself.
  case INSTRUCTION_MODE_REGISTER_TO_MEMORY_TWO_BYTE_DISPLACEMENT:
    fprintf(stderr, "%s mode: Memory Mode, %u byte displacement\n",
            instruction_name, mode);
    destination = memory_displacement_expresion_table[mode][reg_mem];
    n = load_n_bytes_displacement(
        mode, displacement_formated_buffer,
        UNSIGNED_DISPLACEMENT_FORMATED_BUFFER_MAX_LENGTH, destination,
        executable);
    if (n < 0) {
      return EXIT_FAILURE;
    }
    fprintf(stderr, "displacement: %s\n", displacement_formated_buffer);
    destination = displacement_formated_buffer;
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
  n = read_n_bytes_as_number(size, &number, executable);
  if (n < 0) {
    return EXIT_FAILURE;
  }

  printf("%s %s, %u\n", instruction_name, destination, number);
  return EXIT_SUCCESS;
}

struct instruction {
  unsigned char mask;
  unsigned char value;
};

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(
        stderr,
        "%s expects exactly one 8086 executable path %d arguments were given\n",
        argv[0], argc - 1);
    return EXIT_FAILURE;
  }
  fprintf(stderr, "disassembling file %s\n", argv[1]);
  FILE *executable = fopen(argv[1], "rb");
  unsigned char instruction_byte;

  struct instruction instruction_opcode_prefixes[] = {
      // mov immmediate to register
      {.mask = 0b11110000, .value = 0b10110000},
      // mov reg reg/mem with optional displacement
      {.mask = 0b11111100, .value = 0b10001000},
      // mov immediate to memory/reg with optional displacement
      {.mask = 0b11111110, .value = 0b11000110},
      // add reg reg/mem with optional displacement
      {.mask = 0b11111100, .value = 0b00000000},
      // add immmediate to register
      {.mask = 0b11111100, .value = 0b10000000},

  };

  char *instruction_names[] = {"mov", "mov", "mov", "add", "add"};

  int (*decoders[])(unsigned char instruction_byte, FILE *executable,
                    const char *instruction_name) = {
      decode_instruction_immediate_to_reg,        // mov ax, 6
      decode_instruction_reg_mem_reg,             // mov ax, [bp + 2]
      decode_instruction_immediate_to_memory_reg, // mov [bp + 2], 7
      decode_instruction_reg_mem_reg,             // add ax, [bp +2]
      decode_instruction_immediate_to_memory_reg, // add [bp + 2], 7
  };

  size_t decoders_count = sizeof(decoders) / sizeof(decoders[0]);

  int exit_code = EXIT_SUCCESS;
  printf("bits 16\n\n");
  while (fread(&instruction_byte, sizeof(instruction_byte), 1, executable) ==
         1) {

    debug_byte_as_binary("first instruction byte as binary:", instruction_byte);
    int found = 0;
    for (int i = 0; i < decoders_count; i++) {
      unsigned char opcode_mask = instruction_opcode_prefixes[i].mask;
      unsigned char opcode_prefix = instruction_opcode_prefixes[i].value;
      debug_byte_as_binary("trying following prefix:", opcode_prefix);
      if ((instruction_byte & opcode_mask) == opcode_prefix) {
        found = 1;
        exit_code =
            decoders[i](instruction_byte, executable, instruction_names[i]);
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

exit:
  fclose(executable);
  return exit_code;
}
