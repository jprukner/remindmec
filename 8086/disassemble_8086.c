// main takes one argument and that is a path to binary file of 8086 executable.
// It prints out assembly instructions in a format that can be again fed to nasm
// assembler.
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
  printf("%s", output);
  return;
}

void print_bytes_as_binary(const unsigned char bytes[], uint64_t count) {
  for (uint64_t i = 0; i < count; i++) {
    print_byte_as_binary(bytes[i]);
    printf(" ");
  }
  printf("\n");
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf(
        "%s expects exactly one 8086 executable path %d arguments were given\n",
        argv[0], argc - 1);
    return EXIT_FAILURE;
  }
  printf("disassembling file %s\n", argv[1]);
  FILE *executable = fopen(argv[1], "rb");
  unsigned char instruction[2];
  size_t n = fread(&instruction, sizeof(instruction), 1, executable);
  if (n != 1) {
    printf(
        "we expected to read one instruction but %lu instructions were read\n",
        n);
    goto exit_failure;
  }
  print_bytes_as_binary(instruction, 2);

exit_success:
  fclose(executable);
  return EXIT_SUCCESS;
exit_failure:
  fclose(executable);
  return EXIT_FAILURE;
}