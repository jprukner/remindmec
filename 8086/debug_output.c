#include <stdint.h>
#include <stdio.h>

void print_byte_as_binary(uint8_t input) {
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

void debug_byte_as_binary(const char *message, uint8_t byte) {
  fprintf(stderr, message);
  print_byte_as_binary(byte);
  fprintf(stderr, "\n");
}

void debug_uint16_as_binary(const char *message, uint16_t word) {
  fprintf(stderr, message);
  uint8_t byte = (uint8_t)((word & 0xff00) >> 8);
  print_byte_as_binary(byte);
  fprintf(stderr, "|");
  byte = (uint8_t)(word & 0x00ff);
  print_byte_as_binary(byte);
  fprintf(stderr, "\n");
}
