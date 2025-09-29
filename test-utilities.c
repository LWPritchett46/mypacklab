// Application to test unpack utilities
// PackLab - CS213 - Northwestern University

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unpack-utilities.h"


int test_lfsr_step(void) {
  // A properly created LFSR should do two things
  //  1. It should generate specific new state based on a known initial state
  //  2. It should iterate through all 2^16 integers, once each (except 0)

  // Create an array to track if the LFSR hit each integer (except 0)
  // 2^16 (65536) possibilities
  bool* lfsr_states = malloc_and_check(65536);
  memset(lfsr_states, 0, 65536);

  // Initial 16 LFSR states
  uint16_t correct_lfsr_states[16] = {
    0x1337, 0x099B, 0x84CD, 0x4266,
    0x2133, 0x1099, 0x884C, 0xC426,
    0x6213, 0xB109, 0x5884, 0x2C42,
    0x1621, 0x0B10, 0x8588, 0x42C4
  };

  // Step the LFSR until a state repeats
  bool repeat        = false;
  size_t steps       = 0;
  uint16_t new_state = 0x1337; // known initial state
  while (!repeat) {

    // Iterate LFSR
    steps++;
    new_state = lfsr_step(new_state);

    // Check if this state has already been reached
    repeat = lfsr_states[new_state];
    lfsr_states[new_state] = true;

    // Check first 16 LFSR steps
    if (steps < 16) {
      if (new_state != correct_lfsr_states[steps]) {
        printf("ERROR: at step %lu, expected state 0x%04X but received state 0x%04X\n",
            steps, correct_lfsr_states[steps], new_state);
        free(lfsr_states);
        return 1;
      }
    }
  }

  // Check that all integers were hit. Should take 2^16 (65536) steps (2^16-1 integers, plus a repeat)
  if (steps != 65536) {
    printf("ERROR: expected %d iterations before a repeat, but ended after %lu steps\n", 65536, steps);
    free(lfsr_states);
    return 1;
  }

  // Cleanup
  free(lfsr_states);
  return 0;
}

// Here's an example testcase
// It's written for the `calculate_checksum()` function, but the same ideas
//  would work for any function you want to test
// Feel free to copy it and adapt it to create your own tests
int example_test(void) {
  // Create input data to test with
  // If you wanted to test a header, these would be bytes of the header with
  //    meaningful bytes in appropriate places
  // If you want to test one of the other functions, they can be any bytes
  uint8_t input_data[] = {0xFF, 0xFF, 0xFF, };

  // Create an "expected" result to compare against
  // If you're testing header parsing, you will likely need one of these for
  //    each config field. If you're testing decryption or decompression, this
  //    should be an array of expected output_data bytes
  uint16_t expected_checksum_value = 0x02FD;

  // Actually run your code
  // Note that `sizeof(input_data)` actually returns the number of bytes for the
  //    array because it's a local variable (`sizeof()` generally doesn't return
  //    buffer lengths in C for arrays that are passed in as arguments)
  uint16_t calculated_checksum_value = calculate_checksum(input_data, sizeof(input_data));

  // Compare the results
  // This might need to be multiple comparisons or even a loop that compares many bytes
  // `memcmp()` in the C standard libary might be a useful function here!
  // Note, you don't _need_ the CHECK() functions like we used in CS211, you
  //    can just return 1 then print that there was an error
  if (calculated_checksum_value != expected_checksum_value) {
    // Test failed! Return 1 to signify failure
    return 1;
  }

  // Test succeeded! Return 0 to signify success
  return 0;
}

int parse_header_test(void) {
  uint8_t input_data[] = {0x02, 0x13, 0x03, 0x00, 
    0x67, 0x45, 0x23, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x11, 0x11, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00};

  packlab_config_t config;

  parse_header(input_data, 20, &config);
  if (!config.is_valid) {
    printf("ERROR: header is invalid!\n");
    return 1;
  }

  if (config.orig_data_size != 0x0000000001234567) {
    printf("ERROR: expected original data size is 0x%16lX, got 0x%16lX\n", (uint64_t)0x0000000001234567, config.orig_data_size);
    return 1;
  }

  if (config.data_size != 0x0000000011111111) {
    printf("ERROR: expected data size is 0x%16lX, got 0x%16lX\n", (uint64_t)0x0000000011111111, config.data_size);
    return 1;
  }

  return 0;
}

int decompress_test(void) {
  uint8_t input_data[] = {0x01, 0x07, 0x42};
  uint8_t output_data[5];

  uint8_t dict[] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                  0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F};

  decompress_data(input_data, 3, output_data, 5, dict);

  if (output_data[0] != 0x01) {
    printf("ERROR: Expected value 0x01, got %2x\n", output_data[0]);
    return 1;
  }
  if (output_data[1] != 0x32) {
    printf("ERROR1: Expected value 0x32, got 0x%2x\n", output_data[1]);
    return 1;
  }
  if (output_data[2] != 0x32) {
    printf("ERROR2: Expected value 0x32, got 0x%2x\n", output_data[1]);
    return 1;
  }
  if (output_data[3] != 0x32) {
    printf("ERROR3: Expected value 0x32, got 0x%2x\n", output_data[1]);
    return 1;
  }
  if (output_data[4] != 0x32) {
    printf("ERROR4: Expected value 0x32, got 0x%2x\n", output_data[1]);
    return 1;
  }

  return 0;
}

int float_streams_test(void) {
  uint8_t input_signfrac[] = {0x00, 0x00, 0x16};
  uint8_t input_exp[] = {0x87};
  uint8_t output[4];

  join_float_array(input_signfrac, 3, input_exp, 1, output, 4);

  if (output[0] != 0x00) {
    printf("ERROR\n");
    return 1;
  }
  if (output[1] != 0x00) {
    printf("ERROR\n");
    return 1;
  }
  if (output[2] != 0x96) {
    printf("ERROR\n");
    return 1;
  }
  if (output[3] != 0x43) {
    printf("ERROR\n");
    return 1;
  }

  return 0;
}

int main(void) {

  // Test the LFSR implementation
  int result = test_lfsr_step();
  if (result != 0) {
    printf("Error when testing LFSR implementation\n");
    return 1;
  }

  // TODO - add tests here for other functionality
  // You can craft arbitrary array data as inputs to the functions
  // Parsing headers, checksumming, decryption, and decompressing are all testable

  // Here's an example test
  // Note that it's going to fail until you implement the `calculate_checksum()` function
  result = example_test();
  if (result != 0) {
    // Make sure to print the name of which test failed, so you can go find it and figure out why
    printf("ERROR: example_test_setup failed\n");
    return 1;
  }

  result = parse_header_test();
  if (result != 0) {
    printf("ERROR: parse header test failed\n");
    return 1;
  }

  result = decompress_test();
  if (result != 0) {
    printf("ERROR: decompression test failed\n");
    return 1;
  }

  result = float_streams_test();
  if (result != 0) {
    printf("ERROR: float stream test failed\n");
    return 1;
  }

  printf("All tests passed successfully!\n");
  return 0;
}

