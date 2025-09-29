// Utilities for unpacking files
// PackLab - CS213 - Northwestern University

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unpack-utilities.h"


// --- public functions ---

void error_and_exit(const char* message) {
  fprintf(stderr, "%s", message);
  exit(1);
}

void* malloc_and_check(size_t size) {
  void* pointer = malloc(size);
  if (pointer == NULL) {
    error_and_exit("ERROR: malloc failed\n");
  }
  return pointer;
}

void parse_header(uint8_t* input_data, size_t input_len, packlab_config_t* config) {

  config->is_valid = true;

  // Headers are always of size >= 20, anything less is invalid
  if (input_len < 20) {
    config->is_valid = false;
    return;
  }

  // Check for correct Magic and Version numbers
  if (input_data[0] != 0x02 || input_data[1] != 0x13 || input_data[2] != 0x03) {
    config->is_valid = false;
    return;
  } 

  // Isolate flag byte
  uint8_t flags = input_data[3];
  
  // Read byte bitwise to set config flags
  config->is_compressed   = (bool) (flags & 0x80);
  config->is_encrypted    = (bool) (flags & 0x40);
  config->is_checksummed  = (bool) (flags & 0x20);
  config->should_continue = (bool) (flags & 0x10);
  config->should_float    = (bool) (flags & 0x08);
  config->should_float3   = (bool) (flags & 0x04);

  // Read and assign data size
  uint64_t temp_size = 0;
  for (int i = 4; i < 12; i++) {
    temp_size += (uint64_t)input_data[i] << (8 * (i - 4));
  }
  config->orig_data_size = (temp_size);

  temp_size = 0;
  for (int i = 12; i < 20; i++) {
    temp_size += (uint64_t)input_data[i] << (8 * (i - 12));
  }
  config->data_size      = (temp_size);

  // Header length is 20 bytes + 16 optional dictionary bytes + 2 optional checksum bytes
  config->header_len = 20 + (DICTIONARY_LENGTH * (int)config->is_encrypted) + (2 * (int)config->is_checksummed);

  // Check again that input file is not too small
  if (input_len < config->header_len) {
    config->is_valid = false;
    return;
  }

  // Read dictionary and checksum data, if applicable
  if (config->is_compressed) {
    for (int i = 0; i < DICTIONARY_LENGTH; i++) {
      config->dictionary_data[i] = input_data[i + 20];
    }

    if(config->is_checksummed) {
      uint16_t temp_checksum = ((uint16_t)input_data[DICTIONARY_LENGTH + 20] * 256) + 
                               ((uint16_t)input_data[DICTIONARY_LENGTH + 21]);
      config->checksum_value = (temp_checksum);
    }
  } else if (config->is_checksummed) {
    uint16_t temp_checksum = ((uint16_t)input_data[20] * 256) + 
                             ((uint16_t)input_data[21]);
    config->checksum_value = (temp_checksum);
  }

  return;
}

uint16_t calculate_checksum(uint8_t* input_data, size_t input_len) {
  uint16_t checksum = 0;

  for (int i = 0; i < input_len; i++) {
    checksum += (uint16_t)input_data[i];
  }

  return checksum;
}

uint16_t lfsr_step(uint16_t oldstate) {

  bool bit_0 =  (bool) (oldstate & 0x0001);
  bool bit_6 =  (bool) (oldstate & 0x0040);
  bool bit_9 =  (bool) (oldstate & 0x0200);
  bool bit_13 = (bool) (oldstate & 0x2000);

  bool new_bit = bit_0 ^ bit_6 ^ bit_9 ^ bit_13;

  uint16_t newstate = oldstate >> 1;
  newstate += (int) new_bit * 0x8000;

  return newstate;
}

void decrypt_data(uint8_t* input_data, size_t input_len,
                  uint8_t* output_data, size_t output_len,
                  uint16_t encryption_key) {

  // TODO
  // Decrypt input_data and write result to output_data

  uint8_t key_least;
  uint8_t key_most;

  for (int i = 0; i < (input_len - 1); i += 2) {
    encryption_key = lfsr_step(encryption_key);
    key_least = (uint8_t) encryption_key % 256;
    key_most  = (uint8_t) encryption_key / 256;

    output_data[i] = key_least ^ input_data[i];
    output_data[i + 1] = key_most ^ input_data[i + 1];
  }
  if (input_len % 2 != 0) {
    output_data[input_len - 1] = key_least ^ input_data[input_len - 1];
  }

}

size_t decompress_data(uint8_t* input_data, size_t input_len,
                       uint8_t* output_data, size_t output_len,
                       uint8_t* dictionary_data) {

  // TODO
  // Decompress input_data and write result to output_data
  // Return the length of the decompressed data

  int output_index = 0;

  for (int i = 0; i < input_len; i++) {
    if (input_data[i] == 0x07) {

      if (i + 1 >= input_len || input_data[i + 1] == 0x00) {
        output_data[output_index] = 0x07;
        output_index++;
      } else {
        int repeat_count = input_data[i + 1] / 16;
        int dict_index = input_data[i + 1] % 16;

        for (int j = 0; j < repeat_count; j++) {
          output_data[output_index] = dictionary_data[dict_index];
          output_index++;
        }
      }

      i++;
    } else {
      output_data[output_index] = input_data[i];
      output_index++;
    }
  }

  return output_index;
}

void join_float_array(uint8_t* input_signfrac, size_t input_len_bytes_signfrac,
                      uint8_t* input_exp, size_t input_len_bytes_exp,
                      uint8_t* output_data, size_t output_len_bytes) {

  // TODO
  // Combine two streams of bytes, one with signfrac data and one with exp data,
  // into one output stream of floating point data
  // Output bytes are in little-endian order

}
/* End of mandatory implementation. */

/* Extra credit */
void join_float_array_three_stream(uint8_t* input_frac,
                                   size_t   input_len_bytes_frac,
                                   uint8_t* input_exp,
                                   size_t   input_len_bytes_exp,
                                   uint8_t* input_sign,
                                   size_t   input_len_bytes_sign,
                                   uint8_t* output_data,
                                   size_t   output_len_bytes) {

  // TODO
  // Combine three streams of bytes, one with frac data, one with exp data,
  // and one with sign data, into one output stream of floating point data
  // Output bytes are in little-endian order

}

