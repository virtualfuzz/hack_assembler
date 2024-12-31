#include "compiler.h"
#include "create_hashmaps.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

// Implements compiler related functions
// Including the compile_to_file which takes a file and
// outputs the compiled file on another
void to_binary(char *dest, unsigned short n);

struct symbol {
  long location;
  char *a_or_label_value;
};

const unsigned int MAX_A_VALUE = 32767;

// Compiles an input file to an output file
// First pass
// if a instruction
//    known symbol => just compiile it
//    unknown symbol => rememeber this place/symbol and come back later
// if label => update symbol table

// Second pass
// for every a instruction not compiled =>
//    if found in symbol table => compile it using the saved place/symbol
//    if unknown => new variable starting at position 16
bool compile_to_file(FILE *input, FILE *output) {
  bool success = true;
  char *line = NULL;
  size_t buffer_size = 0;
  size_t current_line = 1;
  unsigned short instruction_memory_address = 0;

  struct hashmap *predefined_hashmap = create_predefined_hashmap();
  struct hashmap *symbol_hashmap = create_empty_compiled_hashmap();
  struct hashmap *comp_hashmap = create_comp_hashmap();
  struct hashmap *jump_hashmap = create_jump_hashmap();
  struct array_list unhandled_symbols = array_list_create(50);

  unsigned long long currently_allocated;
  char *a_or_label_value = NULL;

  // Loop though each line
  while ((getline(&line, &buffer_size, input)) != -1) {
    // Initialise the values we are going to parse
    enum instruction instruction_parsed = NONE;

    unsigned long long to_allocate;
    if (__builtin_umulll_overflow(sizeof(char), buffer_size, &to_allocate)) {
      printf("Line %zu is way too big (%zu) and will overflow\n", current_line,
             buffer_size);
      success = false;
      break;
    }

    // If we need to allocate more reallocate
    if (a_or_label_value == NULL || currently_allocated < to_allocate) {
      free(a_or_label_value);
      a_or_label_value = (char *)malloc(to_allocate);
      if (a_or_label_value == NULL) {
        perror("Error while allocating memory for a_or_label_value:");
        success = false;
        break;
      }

      currently_allocated = to_allocate;
    }

    strcpy(a_or_label_value, "");

    // C instructions
    struct c_instruction_value dest = {"", false};
    struct c_instruction_value comp = {"", false};
    struct c_instruction_value jump = {"", false};

    if (parse_line(line, &instruction_parsed, a_or_label_value, &dest, &comp,
                   &jump) != true) {
      printf("at line %zu\n", current_line);
      success = false;
      break;
    };

    if (compile_instruction(output, instruction_parsed, a_or_label_value, dest,
                            comp, jump, predefined_hashmap, comp_hashmap,
                            jump_hashmap, &instruction_memory_address,
                            &unhandled_symbols, symbol_hashmap) != true) {
      printf("at line %zu\n", current_line);
      success = false;
      break;
    };

    current_line++;
  }

  // Compile unhandled symbols
  size_t variable_address = 16;
  while (unhandled_symbols.length != 0) {
    struct symbol *unhandled = array_list_get(unhandled_symbols, 0);
    const struct compiled_instruction *compiled_symbol = hashmap_get(
        symbol_hashmap, &(struct compiled_instruction){
                            .original = unhandled->a_or_label_value});

    // Compile unhandled symbol
    const char *compiled;
    fseek(output, unhandled->location, SEEK_SET);
    if (compiled_symbol != NULL)
      compiled = compiled_symbol->compiled;
    else {
      // Convert the memory address to binary
      char *compiled_variable = malloc(sizeof(char) * 16);
      strcpy(compiled_variable, "");
      to_binary(compiled_variable, variable_address);

      // Put it inside of the symbol hashmap
      hashmap_set(symbol_hashmap,
                  &(struct compiled_instruction){
                      .original = strdup(unhandled->a_or_label_value),
                      .compiled = compiled_variable});

      // Compile and update variable address
      compiled = compiled_variable;
      variable_address++;
    }

    // Write to file
    fprintf(output, "%s\n", compiled);

    // Free it/Remove it
    free(unhandled->a_or_label_value);
    free(unhandled);
    array_list_remove(&unhandled_symbols, 0);
  }

  // Loop over each symbol in the hashmap to free it
  size_t iter = 0;
  void *item;
  while (hashmap_iter(symbol_hashmap, &iter, &item)) {
    struct compiled_instruction *symbol = item;
    free(symbol->compiled);
    free(symbol->original);
  }

  cleanup(NULL, NULL, line, NULL, comp_hashmap, jump_hashmap, a_or_label_value,
          predefined_hashmap, &unhandled_symbols, symbol_hashmap);
  return success;
}

// Compile the parsed instruction into the output file
bool compile_instruction(
    FILE *output_file, const enum instruction instruction_parsed,
    char a_or_label_value[], struct c_instruction_value dest,
    struct c_instruction_value comp, struct c_instruction_value jump,
    struct hashmap *predefined_hashmap, struct hashmap *comp_hashmap,
    struct hashmap *jump_hashmap, unsigned short *instruction_memory_address,
    struct array_list *unhandled_symbols, struct hashmap *symbol_hashmap) {

  // TODO: Temporary fix because
  // "Label followed by a declaration is a C23 extension"
  unsigned long a_value_long;
  char *end;
  char *compiled;
  struct compiled_instruction *label;

  switch (instruction_parsed) {
  case LABEL:
    // Compile the memory address
    compiled = malloc(sizeof(char) * 16);
    strcpy(compiled, "");
    to_binary(compiled, *instruction_memory_address);

    // Put it inside of the symbol hashmap
    hashmap_set(symbol_hashmap, &(struct compiled_instruction){
                                    .original = strdup(a_or_label_value),
                                    .compiled = compiled});
    break;
  case A_INSTRUCTION:
    // Try to convert to number, if it fails, it's in the symbol table
    a_value_long = strtoul(a_or_label_value, &end, 10);

    fprintf(output_file, "0");

    // If the a_value is a memory address (number)
    if (!*end) {
      // Check if number is too big since it will overflow if it is bigger
      if (MAX_A_VALUE < a_value_long) {
        printf("ERROR: The value provided in the A instruction will overflow "
               "(max value: %i)\n If this value is supposed to be supported, "
               "please report this as a bug!",
               MAX_A_VALUE);
        return false;
      }

      // Convert the number to binary
      char output[15] = "";
      to_binary(output, a_value_long);
      fprintf(output_file, "%s", output);
    } else {
      // a_value is a symbol/label/variable

      // Query the precompiled hashmap
      const struct compiled_instruction *precompiled_a = hashmap_get(
          predefined_hashmap,
          &(struct compiled_instruction){.original = a_or_label_value});

      if (precompiled_a != NULL)
        fprintf(output_file, "%s", precompiled_a->compiled);
      else {
        // Not in precompiled hashmap, search the symbol hashmap
        const struct compiled_instruction *compiled_a = hashmap_get(
            symbol_hashmap,
            &(struct compiled_instruction){.original = a_or_label_value});

        if (compiled_a != NULL)
          fprintf(output_file, "%s", compiled_a->compiled);
        else {
          // Symbol not found in precompiled/symbol hashmap
          // Must be a variable/label that was not found yet
          struct symbol *symbol_malloc = malloc(sizeof(struct symbol));
          symbol_malloc->a_or_label_value = strdup(a_or_label_value);
          symbol_malloc->location = ftell(output_file);

          array_list_push(unhandled_symbols, symbol_malloc);

          // Notice: Workaround since otherise we overwrite the other lines
          fprintf(output_file, "               ");
        }
      }
    }

    // Update instruction memory address
    fprintf(output_file, "\n");
    (*instruction_memory_address)++;
    break;
  case C_INSTRUCTION:
    fprintf(output_file, "111");

    // Compile comp part
    const struct compiled_instruction *compiled_comp = hashmap_get(
        comp_hashmap, &(struct compiled_instruction){.original = comp.value});

    // Check if comp exists (it needs to)
    if (compiled_comp == NULL) {
      printf("ERROR: Unknown comp value (%s) of C instruction", comp.value);
      return false;
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
          printf("ERROR: Got A twice in the dest part of a C instruction "
                 "(dest: %s)",
                 dest.value);
          return false;
        }

        compiled_dest[0] = '1';
        break;
      case 'D':
        if (compiled_dest[1] == '1') {
          printf("ERROR: Got D twice in the dest part of a C instruction "
                 "(dest: %s)",
                 dest.value);
          return false;
        }

        compiled_dest[1] = '1';
        break;
      case 'M':
        if (compiled_dest[2] == '1') {
          printf("ERROR: Got M twice in the dest part of a C instruction "
                 "(dest: %s)",
                 dest.value);
          return false;
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
      printf("ERROR: Unknown jump value (%s) in C instruction", jump.value);
      return false;
    }

    fprintf(output_file, "\n");
    (*instruction_memory_address)++;
    break;
  case NONE:
    break;
  default:
    printf("IMPOSSIBLE ERROR: Code that should be impossible to reach has been "
           "reached");
    return false;
  }

  return true;
}

// Convert to binary and send to stream
// A instruction specific since we also print the zeros
void to_binary(char *dest, unsigned short n) {
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
  char temp[2] = ""; // One character string

  for (short j = 14; j >= 0; j--) {
    snprintf(temp, 2, "%d", binary_num[j]);
    strcat(dest, temp);
  }
}
