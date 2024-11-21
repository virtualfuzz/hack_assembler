#include <ctype.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void usage(char *program);
void update_labels(FILE *fp, FILE *output);
void error(char *type, char *message, ...);
void parse_options(int argc, char **argv, bool *force, char **source_filename,
                   char **output_filename);
void open_compiled_file(const char *source_filename, char **output_filename,
                        const bool force, FILE **write_to);
enum instruction {
  NONE,
  FINISHED_VALUE,
  FINISHED_INSTRUCTION,
  SLASH_ONE,
  SLASH_TWO,
  LABEL,
  A_INSTRUCTION,
  C_INSTRUCTION
};

const unsigned int MAX_A_VALUE = 32767;

int main(int argc, char **argv) {
  bool force = false;
  char *source_filename = NULL;
  char *output_filename = NULL;

  parse_options(argc, argv, &force, &source_filename, &output_filename);

  // Try to open source file
  FILE *assembly;
  assembly = fopen(source_filename, "r");
  if (assembly == NULL) {
    error("FILE", "\"%s\" doesn't exist\n", source_filename);
  }

  // Update write_to variable and
  // set it to the pointer of the file where we are going to compile
  FILE *write_to;
  open_compiled_file(source_filename, &output_filename, force, &write_to);

  // First pass: Only update the symbol table
  update_labels(assembly, write_to);

  // Close file
  fclose(assembly);

  // Exit successfully
  exit(EXIT_SUCCESS);
}

// TODO: Everytime it errors out, properly finish the thing to prevent memory
// leaks
// TODO: Create a cleanup function that is runned by every function that exits
// the program
void error(char *type, char *message, ...) {
  fprintf(stderr, "ERROR: %s", type);

  va_list arg;
  va_start(arg, message);
  vfprintf(stderr, message, arg);
  va_end(arg);

  exit(EXIT_FAILURE);
}

// Print usage
void usage(char *program) {
  printf("USAGE: FIXME Usage: %s [-f] [-i input] [-o output]\n", program);
  printf("USAGE: %s foo.asm foo.hack\n", program);
  printf("Creates a file named foo.hack that can be "
         "executed on the Hack computer.\n");
}

void open_compiled_file(const char *source_filename, char **output_filename,
                        const bool force, FILE **write_to) {
  if (output_filename == NULL) {
    // Copy into a new string source filename
    // Reason: We used to output_filename = source_filename
    // but it will modify source too
    size_t length = strlen(source_filename);
    size_t allocation_size = length + 2;

    *output_filename = (char *)malloc(allocation_size);
    if (*output_filename == NULL) {
      error("MEMORY ",
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
    error("FILE ",
          "%s already exists, use the --force flag to overwrite file\n",
          *output_filename);
  }

  // Create compiled code file
  *write_to = fopen(*output_filename, "w");
  if (*write_to == NULL) {
    error("FILE ",
          "Failed to create file %s maybe a directory doesn't exist...",
          *output_filename);
  }
}

// Parse options from cmd line args and update force, source_filename and
// output_filename accordingly
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

// Take a file as input and add labels to the symbol table if missing
void update_labels(FILE *fp, FILE *output) {
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  size_t current_line = 1;

  // Loop though each line
  while ((read = getline(&line, &len, fp)) != -1) {

    size_t line_len = strlen(line);
    enum instruction parsing_status =
        NONE; // Parsing status, changes throughout line
    enum instruction instruction_parsed =
        NONE; // Type of the instruction parsed in the current line
    char *pointer_to_value_changed = NULL; // pointer to the current value
                                           // changed (A value/dest/comp/jump)
    char *dest = NULL;                     // destination part of C instruction
    char *comp = NULL;                     // comp part of C instruction
    char *jump = NULL;                     // jump part of C instruction

    // Loop though each character in line
    for (size_t i = 0; i < line_len; i++) {
      // Check for current character and either add it to value or
      // set the current instruction
      switch (line[i]) {
      case '@':
        if (parsing_status == NONE) {
          parsing_status = A_INSTRUCTION;
          instruction_parsed = A_INSTRUCTION;

          // Create new string with
          // the max size set to 6 or line_len (which ever is smaller)
          unsigned short allocation_size = line_len < 6 ? line_len : 6;
          pointer_to_value_changed = (char *)malloc(allocation_size);
          strcpy(pointer_to_value_changed, "");
        } else {
          error("SYNTAX ", "Unexpected @ at line %zu\n", current_line);
        }
        break;
      case '(':
        if (parsing_status == NONE) {
          parsing_status = LABEL;
          instruction_parsed = LABEL;
        } else {
          fclose(fp);
          free(line);
          error("SYNTAX ", "Unexpected ( at line %zu\n", current_line);
        }
        break;
      case ')':
        if (parsing_status != FINISHED_INSTRUCTION ||
            parsing_status == FINISHED_VALUE) {
          parsing_status = FINISHED_INSTRUCTION;
        } else {
          fclose(fp);
          free(line);
          error("SYNTAX ", "Unexpected ) at line %zu\n", current_line);
        }
        break;
      case '/':
        switch (parsing_status) {
        case SLASH_ONE:
          parsing_status = SLASH_TWO;
          break;
        case C_INSTRUCTION:
        case FINISHED_VALUE:
          if (instruction_parsed == C_INSTRUCTION) {
            // dest -> dest
            if ((dest == NULL || strcmp(dest, "") != 0) &&
            (jump == NULL || strcmp(jump, "") != 0) &&
            (comp != NULL && strcmp(comp, "") != 0)) {
              parsing_status = SLASH_ONE;
              break;
            }
          }

          error("SYNTAX ", "Unexpected character (%c) on line %zu\n", line[i],
                current_line);
          break;

          // okay when to break
          // if smth != null and string not empty then good
          //
          // check if comp is atleast set, otherise then bug
          // check if every value that has been intialised is set (to prevent =)

        case FINISHED_INSTRUCTION:
        case NONE:
          parsing_status = SLASH_ONE;
          break;
        // TODO: For C instructions we just need to make sure that there is a
        // comp
        default:
          error("SYNTAX ", "Unexpected character (%c) on line %zu\n", line[i],
                current_line);
        }
        break;
      case '0':
      case '1':
      case '-':
      case 'D':
      case 'A':
      case '!':
      case 'M':
        if (parsing_status == NONE) {
          parsing_status = C_INSTRUCTION;
          instruction_parsed = C_INSTRUCTION;

          // Create new string with
          // the max size set to 4 or line_len (which ever is smaller)
          dest = (char *)malloc(4);
          pointer_to_value_changed = dest;
        }
        // THIS ALSO GETS RUN IF PARSING STATUS IS C INSTRUCTION WHICH PREVENTS
        // DEFAULT
        __attribute__((fallthrough));
      default:
        // Handle the values provided in the instruction
        if (parsing_status == A_INSTRUCTION && isdigit(line[i])) {
          // Max size of A instruction (since it can overflow since we are using
          // 15 bytes to represent the number)
          if (4 < strlen(pointer_to_value_changed))
            error(
                "A INSTRUCTION SIZE ",
                "The value provided in the A instruction will overflow (max value: %i),\n\
If this value is supposed to be supported in a future Hack version, please report to developer!\n",
                MAX_A_VALUE);

          strncat(pointer_to_value_changed, &line[i], 1);

        } else if (isspace(line[i])) {
          // This part of the code allows for spacing like @      19 or MD =  0
          if (pointer_to_value_changed != NULL &&
              strcmp(pointer_to_value_changed, "") != 0) {
            switch (parsing_status) {
            case A_INSTRUCTION:
              parsing_status = FINISHED_INSTRUCTION;
              break;
            case C_INSTRUCTION:
              // FIXME: COMMENTS IN C INSTRUCTIONS DONT WORK :skull:
              // FIXME: Comments are mendataory, we don't care if we are doing
              // shit (YOU GET IT?)
              parsing_status = FINISHED_VALUE;
              break;
            case LABEL:
              parsing_status = FINISHED_VALUE;
              break;
            case SLASH_ONE:
              error("SYNTAX ", "Unexpected character (%c) on line %zu\n",
                    line[i], current_line);
              break;
            default:
              break;
            }
          }
        } else if ((parsing_status == FINISHED_VALUE &&
                    instruction_parsed == C_INSTRUCTION) ||
                   parsing_status == C_INSTRUCTION) {
          switch (line[i]) {
          case '=':
            // If the pointer to value changed is dest (value supposed to be
            // before =)
            if (pointer_to_value_changed == dest) {
              // Create a new string with the size set to 4 or length of line
              const unsigned short allocation_size =
                  line_len + 1 < 4 ? line_len + 1 : 4;
              comp = (char *)malloc(allocation_size);
              pointer_to_value_changed = comp;
              strcpy(pointer_to_value_changed, "");
              parsing_status = C_INSTRUCTION;
            } else
              error("SYNTAX ", "Unexepected character (%c) on line %zu\n",
                    line[i], current_line);
            break;
          case ';':
            if (pointer_to_value_changed == dest) {
              comp = dest;
            } else if (pointer_to_value_changed == comp) {
            } else
              error("SYNTAX ", "Unexepected character (%c) on line %zu\n",
                    line[i], current_line);

            jump = (char *)malloc(4);
            pointer_to_value_changed = jump;
            strcpy(pointer_to_value_changed, "");
            parsing_status = C_INSTRUCTION;
            // if pointer == dest -> comp = dest; pointer = jump
            // if pointer == comp -> pointer = jump
            // if pointer == jump -> error
            break;
          default:
            // just add it to the current value?
            if (parsing_status == C_INSTRUCTION)
              strncat(pointer_to_value_changed, &line[i], 1);
            else
              error("SYNTAX ", "Unexepected character (%c) on line %zu\n",
                    line[i], current_line);

            break;
          }
          // we should add the value to pointer current value -> dest
          // we will just check for the length
        } else {
          error("SYNTAX ", "Unexepected character (%c) on line %zu\n", line[i],
                current_line);
        }
      }

      if (parsing_status == SLASH_TWO)
        break;
    }

    // Handle the instruction now that we parsed it
    if (instruction_parsed == A_INSTRUCTION) {
      // Convert string to a long number
      unsigned long a_value = atol(pointer_to_value_changed);

      // Check if number is too big since it will overflow if it is bigger
      if (MAX_A_VALUE < a_value)
        error(
            "A INSTRUCTION SIZE ",
            "The value provided in the A instruction will overflow (max value: %i),\n\
If this value is supposed to be supported in a future Hack version, please report to developer!\n",
            MAX_A_VALUE);

      // Write the instruction to the file
      fprintf(output, "0");                     // Write 0 (a instruction)
      a_instruction_to_binary(output, a_value); // Write the value
      fprintf(output, "\n");                    // Write a new line
    }

    if (dest)
      printf("dest: %s\n", dest);

    if (comp)
      printf("comp: %s\n", comp);

    if (jump)
      printf("jump: %s\n", jump);

    current_line++;
  }

  free(line);
}