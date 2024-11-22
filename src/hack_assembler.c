#include "hack_assembler.h"

int main(int argc, char **argv) {
  // Parse options
  bool force = false;
  char *source_filename = NULL;
  char *output_filename = NULL;

  parse_options(argc, argv, &force, &source_filename, &output_filename);

  // Try to open source file
  FILE *assembly_file = fopen(source_filename, "r");
  if (assembly_file == NULL) {
    error(NULL, NULL, "FILE", "\"%s\" doesn't exist\n", source_filename);
  }

  // Open the compiled file
  FILE *output_file;
  open_compiled_file(assembly_file, source_filename, &output_filename, force,
                     &output_file);

  // Finally start compiling
  compile_to_file(assembly_file, output_file);

  cleanup(assembly_file, output_file, NULL);
  exit(EXIT_SUCCESS);
}

// Clean up memory/opened files
void cleanup(FILE *assembly_file, FILE *output_file, char *line) {
  if (assembly_file)
    fclose(assembly_file);

  if (output_file)
    fclose(output_file);

  if (line)
    free(line);
}

// Cleanup memory, print error, and exit
void error(FILE *assembly_file, FILE *output_file, char *line, const char *type,
           const char *message, ...) {
  cleanup(assembly_file, output_file, line);

  fprintf(stderr, "ERROR: %s", type);

  va_list arg;
  va_start(arg, message);
  vfprintf(stderr, message, arg);
  va_end(arg);

  exit(EXIT_FAILURE);
}

// Print usage
void usage(const char *program) {
  printf("USAGE: FIXME Usage: %s [-f] [-i input] [-o output]\n", program);
  printf("USAGE: %s foo.asm foo.hack\n", program);
  printf("Creates a file named foo.hack that can be "
         "executed on the Hack computer.\n");
}

// Opens/Creates compiled file on the write_to variable
// Update output_filename with the actual filename of the file
void open_compiled_file(FILE *assembly_file, const char *source_filename,
                        char **output_filename, const bool force,
                        FILE **write_to) {
  if (output_filename == NULL) {
    // Copy into a new string source filename
    // Reason: We used to output_filename = source_filename
    // but it will modify source too
    size_t length = strlen(source_filename);
    size_t allocation_size = length + 2;

    *output_filename = (char *)malloc(allocation_size);
    if (*output_filename == NULL) {
      error(assembly_file, NULL, NULL, "MEMORY ",
            "Failed to allocate %zu bytes for hack_save_file string\n",
            allocation_size);
    }

    *output_filename = strcpy(*output_filename, source_filename);

    // Remove .asm from the end of the file if it exists
    if (4 < length) {

      // Get last 4 characters of hack_save_file (first arg, asm filename)
      char to_compare[5];
      strncpy(to_compare, *output_filename + length - 4, 4);

      if (strcmp(to_compare, ".asm") == 0) {
        // Remove the last 4 characters (.asm)
        *output_filename[length - 4] = '\0';
      }
    }

    // Add .hack to the filename
    strcat(*output_filename, ".hack");
  }

  // If file already exists and we allow overwriting
  if (access(*output_filename, F_OK) != -1 && force == false) {
    error(assembly_file, NULL, "FILE ",
          "%s already exists, use the --force flag to overwrite file\n",
          *output_filename);
  }

  // Create compiled code file
  *write_to = fopen(*output_filename, "w");
  if (*write_to == NULL) {
    error(assembly_file, NULL, "FILE ",
          "Failed to create file %s maybe a directory doesn't exist...",
          *output_filename);
  }
}

// Parse options from cmd line args
// Set force, source_filename and output_filename accordingly
void parse_options(int argc, char **argv, bool *force, char **source_filename,
                   char **output_filename) {
  // Options that our compiler takes
  struct option long_options[] = {{"force", no_argument, 0, 'f'},
                                  {"input", required_argument, 0, 'i'},
                                  {"output", required_argument, 0, 'o'},
                                  {0, 0, 0, 0}};

  // Get options
  int opt;
  while ((opt = getopt_long(argc, argv, "fi:o:", long_options, NULL)) != -1) {
    switch (opt) {
    case 'f':
      *force = true;
      break;
    case 'i':
      *source_filename = optarg;
      break;
    case 'o':
      *output_filename = optarg;
      break;
    case '?':
      usage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  // Source filename is required
  if (*source_filename == NULL) {
    usage(argv[0]);
    exit(EXIT_FAILURE);
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

    compile_instruction(input, output, line, instruction_parsed, a_value, dest, comp,
                        jump);

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
          assembly_file, output_file, line, "A INSTRUCTION SIZE ",
          "The value provided in the A instruction will overflow (max value: %i),\n\
If this value is supposed to be supported in a future Hack version, please report to developer!\n",
          MAX_A_VALUE);

    // Write the instruction to the file
    fprintf(output_file, "0"); // Write 0 (a instruction)
    a_instruction_to_binary(output_file, a_value_long); // Write the value
    fprintf(output_file, "\n");

    break;
  case C_INSTRUCTION:
    if (strcmp(dest.value, "") != 0)
      printf("dest: %s\n", dest.value);

    if (strcmp(comp.value, "") != 0)
      printf("comp: %s\n", comp.value);

    if (strcmp(jump.value, "") != 0)
      printf("jump: %s\n", jump.value);
    break;
  case NONE:
    break;
  default:
    error(assembly_file, output_file, line, "IMPOSSIBLE",
          "Pretty much impossible code has been reached");
    break;
  }
}

// Loop though each character in line and parse instruction
// Set instruction_parsed, a_value, dest, comp, and jump
void parse_line(FILE *assembly_file, FILE *output_file, char *line,
                const size_t current_line, enum instruction *instruction_parsed,
                char a_value[], struct c_instruction_value *dest,
                struct c_instruction_value *comp,
                struct c_instruction_value *jump) {

  // What we are currently doing in the line
  enum instruction parsing_status = NONE;

  // Pointer to the current value being changed
  char *pointer_to_value_changed = NULL;

  // Loop though each character in line
  for (size_t i = 0; i < strlen(line); i++) {
    // Check for current character and either add it to value or
    // set the current instruction
    switch (line[i]) {
    // A instruction
    case '@':
      if (parsing_status == NONE) {
        parsing_status = A_INSTRUCTION;
        *instruction_parsed = A_INSTRUCTION;

        pointer_to_value_changed = a_value;
      } else
        error(assembly_file, output_file, line, "SYNTAX ",
              "Unexpected @ at line %zu\n", current_line);
      break;

    // Labels
    // TODO: Finish label code
    case '(':
      if (parsing_status == NONE) {
        parsing_status = LABEL;
        *instruction_parsed = LABEL;
      } else
        error(assembly_file, output_file, line, "SYNTAX ",
              "Unexpected ( at line %zu\n", current_line);
      break;
    case ')':
      if (parsing_status == LABEL || parsing_status == FINISHED_VALUE) {
        parsing_status = FINISHED_INSTRUCTION;
      } else
        error(assembly_file, output_file, line, "SYNTAX ",
              "Unexpected ) at line %zu\n", current_line);
      break;

    // Comments
    case '/':
      // Switch though the current parsing status to
      // Check if we can actually break out of the loop

      switch (parsing_status) {
      case SLASH_ONE:
        parsing_status = SLASH_TWO;
        break;
      case FINISHED_INSTRUCTION:
      case NONE:
        parsing_status = SLASH_ONE;
        break;

      case C_INSTRUCTION:
      case FINISHED_VALUE:
        // If we are in C_INSTRUCTION and every value is set correctly
        // (if the value is valid it should be set to something)
        if (*instruction_parsed == C_INSTRUCTION) {
          parsing_status = SLASH_ONE;
          break;
        }

        __attribute__((fallthrough));
      default:
        error(assembly_file, output_file, line, "SYNTAX ",
              "Unexpected character (%c) on line %zu\n", line[i], current_line);
      }
      break;

    // C instructions
    case '0':
    case '1':
    case '-':
    case 'D':
    case 'A':
    case '!':
    case 'M':
      // If parsing status is not set
      if (parsing_status == NONE) {
        parsing_status = C_INSTRUCTION;
        *instruction_parsed = C_INSTRUCTION;

        dest->validity = true;
        pointer_to_value_changed = dest->value;
      }

      // Also run default since it is the part of the code that adds to current
      // value
      __attribute__((fallthrough));

    // Handle other characters (mostly appending it the current value)
    default:
      // A instructions
      if (parsing_status == A_INSTRUCTION && isdigit(line[i])) {
        // Max size of A instruction (since it can overflow since we are using
        // 15 bytes to represent the number)
        if (4 < strlen(pointer_to_value_changed))
          error(
              assembly_file, output_file, line, "A INSTRUCTION SIZE ",
              "The value provided in the A instruction will overflow (max value: %i),\n\
If this value is supposed to be supported in a future Hack version, please report to developer!\n",
              MAX_A_VALUE);

        strncat(a_value, &line[i], 1);

        // This part of the code allows for spacing like @      19 or MD =  0
      } else if (isspace(line[i])) {
        // Make sure we at least wrote something
        // before trying to set it to finished value
        if (pointer_to_value_changed != NULL &&
            strcmp(pointer_to_value_changed, "") != 0) {

          // Handle spacing, mostly set it to FINISHED_VALUE
          switch (parsing_status) {
          case A_INSTRUCTION:
            parsing_status = FINISHED_INSTRUCTION;
            break;
          case C_INSTRUCTION:
            parsing_status = FINISHED_VALUE;
            break;
          case LABEL:
            parsing_status = FINISHED_VALUE;
            break;
          case SLASH_ONE:
            error(assembly_file, output_file, line, "SYNTAX ",
                  "Unexpected character (%c) on line %zu\n", line[i],
                  current_line);
            break;
          case NONE:
          case FINISHED_VALUE:
          case FINISHED_INSTRUCTION:
          case SLASH_TWO:
            break;
          }
        }

        // Parse C instructions
      } else if ((parsing_status == FINISHED_VALUE &&
                  *instruction_parsed == C_INSTRUCTION) ||
                 parsing_status == C_INSTRUCTION) {
        switch (line[i]) {
          // dest -> comp
        case '=':
          // If the pointer to value changed is dest
          // (value supposed to be before comp)
          if (pointer_to_value_changed == dest->value) {
            parsing_status = C_INSTRUCTION;

            comp->validity = true;
            pointer_to_value_changed = comp->value;
          } else
            error(assembly_file, output_file, line, "SYNTAX ",
                  "Unexepected character (%c) on line %zu\n", line[i],
                  current_line);

          break;

          // comp -> jump
        case ';':
          // if the current value is dest (we are assuming it's dest at the
          // start) then the actual one is comp since we encountered a ;
          if (pointer_to_value_changed == dest->value)
            comp = dest;
          else if (pointer_to_value_changed != comp->value)
            error(assembly_file, output_file, line, "SYNTAX ",
                  "Unexepected character (%c) on line %zu\n", line[i],
                  current_line);

          parsing_status = C_INSTRUCTION;

          // Set current value to jump
          pointer_to_value_changed = jump->value;
          jump->validity = true;
          break;

          // Another character, add it to current value
        default:
          // Check if we are indeed in a C_INSTRUCTION (not FINISHED_VALUE)
          // And make sure we aren't going to overflow our string (maximum 3)
          if (parsing_status == C_INSTRUCTION &&
              strlen(pointer_to_value_changed) < 3)
            strncat(pointer_to_value_changed, &line[i], 1);
          else
            error(assembly_file, output_file, line, "SYNTAX ",
                  "Unexepected character (%c) on line %zu\n", line[i],
                  current_line);

          break;
        }

        // Unhandled character
      } else
        error(assembly_file, output_file, line, "SYNTAX ",
              "Unexepected character (%c) on line %zu\n", line[i],
              current_line);
    }

    // Break since we are in a comment
    if (parsing_status == SLASH_TWO)
      break;
  }

  // Make sure dest, jump and comp is set correctly
  // (if they are valid they MUST contain something)
  if ((dest->validity == true && strcmp(dest->value, "") == 0) ||
      (jump->validity == true && strcmp(jump->value, "") == 0) ||
      (comp->validity == true && strcmp(comp->value, "") == 0)) {
    error(assembly_file, output_file, line, "SYNTAX ",
          "Unexepected character end of line %zu\n", current_line);
  }
}