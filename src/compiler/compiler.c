#include "compiler.h"
#include <stdlib.h>
#include <string.h>
#include "../helpers.h"

// Implements compiler related functions
// Including the compile_to_file which takes a file and
// outputs the compiled file on another
const unsigned int MAX_A_VALUE = 32767;

// Loop though every line in the file
// Then parse it
// Then compile it
void compile_to_file(FILE *input, FILE *output) {
  char *line = NULL;
  size_t line_length = 0;
  size_t current_line = 1;

  // Loop though each line
  while (getline(&line, &line_length, input) != -1) {
    // Initialise the values we are going to parse
    enum instruction instruction_parsed = NONE;

    char a_value[6] = "";

    // C instructions
    struct c_instruction_value dest = {"", false};
    struct c_instruction_value comp = {"", false};
    struct c_instruction_value jump = {"", false};

    parse_line(input, output, line, current_line, &instruction_parsed, a_value,
               &dest, &comp, &jump);

    compile_instruction(input, output, line, instruction_parsed, a_value, dest,
                        comp, jump);

    current_line++;
  }

  free(line);
}

// Compile the parsed instruction into the output file
void compile_instruction(FILE *assembly_file, FILE *output_file, char *line,
                         const enum instruction instruction_parsed,
                         const char a_value[],
                         const struct c_instruction_value dest,
                         const struct c_instruction_value comp,
                         const struct c_instruction_value jump) {

  // TODO: Temporary fix because
  // "Label followed by a declaration is a C23 extension"
  unsigned long a_value_long;

  switch (instruction_parsed) {
  case A_INSTRUCTION:
    // Convert string to a long number
    a_value_long = atol(a_value);

    // Check if number is too big since it will overflow if it is bigger
    if (MAX_A_VALUE < a_value_long)
      error(
          assembly_file, output_file, line, NULL, "A INSTRUCTION SIZE ",
          "The value provided in the A instruction will overflow (max value: %i),\n\
If this value is supposed to be supported in a future Hack version, please report to developer!\n",
          MAX_A_VALUE);

    // Write the instruction to the file
    fprintf(output_file, "0"); // Write 0 (a instruction)
    a_instruction_to_binary(output_file, a_value_long); // Write the value
    fprintf(output_file, "\n");

    break;
  case C_INSTRUCTION:
    // FIXME: Hey dude don't add me yet cuz im not finished haha
    fprintf(output_file, "111");

    // we store in hashmap and lookup
    if (strcmp(comp.value, "") != 0)
      printf("comp: %s\n", comp.value);

    const unsigned short int dest_size = strlen(dest.value);
    unsigned short int t = 0; // only 3 values amirite?
    if (dest_size != 0) {

      for (unsigned short int i = 0; i < dest_size; i++) {
        switch (dest.value[i]) {
        case 'A':
          t += 100;
          printf("A?");
          break;
        case 'D':
          printf("D");
          t += 10;
          break;
        case 'M':
          printf("M");
          t += 1;
          break;
        }
      }

      //
      // loop over each character
      // A -> 1st char is 1 (add 100)
      // D -> 2nd char is 1 (add 10)
      // M -> 3rd char is 1 (add 1)
      fprintf(output_file, "%i", t);
    } else
      fprintf(output_file, "000");

    if (strcmp(jump.value, "") != 0)
      printf("jump: %s\n", jump.value);

    fprintf(output_file, "\n");
    break;
  case NONE:
    break;
  default:
    error(assembly_file, output_file, line, NULL, "IMPOSSIBLE",
          "Pretty much impossible code has been reached");
    break;
  }
}

// Convert to binary and send to stream
// A instruction specific since we also print the zeros
void a_instruction_to_binary(FILE *stream, unsigned short n) {
  // Initialise the a instruction value to 0 (this way we print the whole thing
  // instead)
  unsigned short binary_num[15] = {0};

  // Convert to binary (going to be saved in the reverse order)
  unsigned short length_binary = 0;
  while (n > 0) {
    binary_num[length_binary] = n % 2;
    n = n / 2;
    length_binary++;
  }

  // Print the entirety of the binary_num into the stream (it's in reverse order
  // tho)
  for (short j = 14; j >= 0; j--)
    fprintf(stream, "%d", binary_num[j]);
}
