#include "compiler.h"
#include "../helpers.h"
#include "create_hashmaps.h"
#include <stdlib.h>
#include <string.h>

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

  // Create hashmaps to compile C instructions
  struct hashmap *comp_hashmap = create_comp_hashmap();
  struct hashmap *jump_hashmap = create_jump_hashmap();

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
               &dest, &comp, &jump, comp_hashmap, jump_hashmap);

    compile_instruction(input, output, line, instruction_parsed, a_value, dest,
                        comp, jump, comp_hashmap, jump_hashmap);

    current_line++;
  }

  hashmap_free(comp_hashmap);
  hashmap_free(jump_hashmap);
  free(line);
}

// Compile the parsed instruction into the output file
void compile_instruction(FILE *assembly_file, FILE *output_file, char *line,
                         const enum instruction instruction_parsed,
                         const char a_value[],
                         const struct c_instruction_value dest,
                         struct c_instruction_value comp,
                         struct c_instruction_value jump,
                         struct hashmap *comp_hashmap,
                         struct hashmap *jump_hashmap) {

  // TODO: Temporary fix because
  // "Label followed by a declaration is a C23 extension"
  unsigned long a_value_long;

  switch (instruction_parsed) {
  case A_INSTRUCTION:
    // Convert string to a long number
    a_value_long = atol(a_value);

    // Check if number is too big since it will overflow if it is bigger
    if (MAX_A_VALUE < a_value_long) {
      cleanup(assembly_file, output_file, line, NULL, comp_hashmap,
              jump_hashmap);
      error(
          "A INSTRUCTION SIZE ",
          "The value provided in the A instruction will overflow (max value: %i),\n\
If this value is supposed to be supported in a future Hack version, please report to developer!\n",
          MAX_A_VALUE);
    }

    // Write the instruction to the file
    fprintf(output_file, "0"); // Write 0 (a instruction)
    a_instruction_to_binary(output_file, a_value_long); // Write the value
    fprintf(output_file, "\n");

    break;
  case C_INSTRUCTION:
    fprintf(output_file, "111");

    // Compile comp part
    const struct compiled_instruction *compiled_comp = hashmap_get(
        comp_hashmap, &(struct compiled_instruction){.original = comp.value});

    // Check if comp exists (it needs to)
    if (compiled_comp == NULL) {
      cleanup(assembly_file, output_file, line, NULL, comp_hashmap,
              jump_hashmap);
      error("TYPE ", "invalid comp value (%s) of c instruction", comp.value);
    }

    fprintf(output_file, "%s", compiled_comp->compiled);

    // Compile dest part

    // Assumes that the parser made sure that the dest.value isn't longer then 3
    const unsigned short dest_size = strlen(dest.value);
    char compiled_dest[] = "000";

    for (unsigned short i = 0; i < dest_size; i++) {
      // Set the parts of compiled_dest depending on which letter we get
      switch (dest.value[i]) {
      case 'A':
        if (compiled_dest[0] == '1') {
          cleanup(assembly_file, output_file, line, NULL, comp_hashmap,
                  jump_hashmap);
          error("TYPE ",
                "Got A twice in the dest part of a C instruction (dest = %s)",
                dest.value);
        }

        compiled_dest[0] = '1';
        break;
      case 'D':
        if (compiled_dest[1] == '1') {
          cleanup(assembly_file, output_file, line, NULL, comp_hashmap,
                  jump_hashmap);
          error("TYPE ",
                "Got D twice in the dest part of a C instruction (dest = %s)",
                dest.value);
        }

        compiled_dest[1] = '1';
        break;
      case 'M':
        if (compiled_dest[2] == '1') {
          cleanup(assembly_file, output_file, line, NULL, comp_hashmap,
                  jump_hashmap);
          error("TYPE ",
                "Got M twice in the dest part of a C instruction (dest = %s)",
                dest.value);
        }

        compiled_dest[2] = '1';
        break;
      }
    }

    fprintf(output_file, "%s", compiled_dest);

    // Compile jump part of C instruction
    const struct compiled_instruction *compiled_jump = hashmap_get(
        jump_hashmap, &(struct compiled_instruction){.original = jump.value});

    // If jump exist and is valid
    if (compiled_jump != NULL)
      fprintf(output_file, "%s", compiled_jump->compiled);

    // If jump is empty
    else if (strcmp(jump.value, "") == 0)
      fprintf(output_file, "000");

    // If jump is invalid
    else {
      cleanup(assembly_file, output_file, line, NULL, comp_hashmap,
              jump_hashmap);
      error("TYPE ", "invalid jump value (%s) of c instruction", jump.value);
    }

    fprintf(output_file, "\n");
    break;
  case NONE:
    break;
  default:
    cleanup(assembly_file, output_file, line, NULL, comp_hashmap, jump_hashmap);
    error("IMPOSSIBLE", "Pretty much impossible code has been reached");
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
