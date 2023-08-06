// main takes one argument and that is a path to binary file of 8086 executable.
// It prints out assembly instructions in a format that can be again fed to nasm
// assembler.
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
  // TODO MAKE THIS CONSTANTST
  const unsigned char opcode_mask = 0xfc;
  const unsigned char direction_mask = 0x02;
  const unsigned char word_mask = 0x01;
  const unsigned char mode_mask = 0xc0;
  const unsigned char register_mask = 0x38;
  const unsigned char register_memory_mask = 0x07;

  char *register_word_map[][8] = {
      {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"},
      {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"},
  };

  int exit_code = EXIT_SUCCESS;
  printf("bits 16\n\n");
  while (fread(&instruction_byte, sizeof(instruction_byte), 1, executable) ==
         1) {

    debug_byte_as_binary("first instruction byte as binary:", instruction_byte);

    unsigned char opcode = instruction_byte & opcode_mask;
    unsigned char direction = (instruction_byte & direction_mask) >> 1;
    unsigned char word = instruction_byte & word_mask;
    debug_byte_as_binary("opcode as binary: ", opcode);

    switch (opcode) {
    case 0x88:
      fprintf(stderr, "it's mov type 'Register/memory to/from register' \n");
      size_t n =
          fread(&instruction_byte, sizeof(instruction_byte), 1, executable);
      if (n != 1) {
        fprintf(
            stderr,
            "expected one byte for mov instruction to be complete, got none\n");
        exit_code = EXIT_FAILURE;
        goto exit;
      }
      debug_byte_as_binary("mov - next byte as binary: ", instruction_byte);
      fprintf(stderr, "direction is %u\n", direction);
      fprintf(stderr, "word is %u\n", word);

      unsigned char mode = instruction_byte & mode_mask;
      switch (mode) {
      case 0xc0:
        fprintf(stderr, "mov mode: Register Mode\n");
        unsigned char reg = (instruction_byte & register_mask) >> 3;
        unsigned char reg_mem = instruction_byte & register_memory_mask;
        debug_byte_as_binary("reg:", reg);
        debug_byte_as_binary("reg/mem:", reg_mem);
        if (direction == 0) {
          unsigned char tmp = reg;
          reg = reg_mem;
          reg_mem = tmp;
        }

        printf("mov %s, %s\n", register_word_map[word][reg],
               register_word_map[word][reg_mem]);

        break;
      default:
        debug_byte_as_binary("unknown mode: ", mode);
        exit_code = EXIT_FAILURE;
        goto exit;
      }
      break;
    default:
      debug_byte_as_binary("unknown opcode: ", opcode);
      exit_code = EXIT_FAILURE;
      goto exit;
    };
    fprintf(stderr, "---- next instruction ----\n");
  }

exit:
  fclose(executable);
  return exit_code;
}
