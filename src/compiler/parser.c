#include "parser.h"
#include "../helpers.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

// Implements the parser, which parses a line
// And outputs the parsed instruction to be compiled
extern const int MAX_A_VALUE;

// Loop though each character in line and parse instruction
// Set instruction_parsed, a_value, dest, comp, and jump
void parse_line(FILE *assembly_file, FILE *output_file, char *line,
                const size_t current_line, enum instruction *instruction_parsed,
                char a_value[], struct c_instruction_value *dest,
                struct c_instruction_value *comp,
                struct c_instruction_value *jump, struct hashmap *comp_hashmap,
                struct hashmap *jump_hashmap) {

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
      } else {
        cleanup(assembly_file, output_file, line, NULL, comp_hashmap, jump_hashmap);
        error("SYNTAX ", "Unexpected @ at line %zu\n", current_line);
      };
      break;

    // Labels
    // TODO: Finish label code
    case '(':
      if (parsing_status == NONE) {
        parsing_status = LABEL;
        *instruction_parsed = LABEL;
      } else {
        cleanup(assembly_file, output_file, line, NULL, comp_hashmap, jump_hashmap);
        error("SYNTAX ", "Unexpected ( at line %zu\n", current_line);
      }
      break;
    case ')':
      if (parsing_status == LABEL || parsing_status == FINISHED_VALUE) {
        parsing_status = FINISHED_INSTRUCTION;
      } else {
        cleanup(assembly_file, output_file, line, NULL, comp_hashmap, jump_hashmap);
        error("SYNTAX ", "Unexpected ) at line %zu\n", current_line);
      }
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
        cleanup(assembly_file, output_file, line, NULL, comp_hashmap, jump_hashmap);
        error("SYNTAX ", "Unexpected character (%c) on line %zu\n", line[i],
              current_line);
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
        if (4 < strlen(pointer_to_value_changed)) {
          cleanup(assembly_file, output_file, line, NULL, comp_hashmap, jump_hashmap);
          error(
              "A INSTRUCTION SIZE ",
              "The value provided in the A instruction will overflow (max value: %i),\n\
If this value is supposed to be supported in a future Hack version, please report to developer!\n",
              MAX_A_VALUE);
        }

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
            cleanup(assembly_file, output_file, line, NULL, comp_hashmap, jump_hashmap);
            error("SYNTAX ", "Unexpected character (%c) on line %zu\n", line[i],
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
          } else {
            cleanup(assembly_file, output_file, line, NULL, comp_hashmap,
                    jump_hashmap);
            error("SYNTAX ", "Unexepected character (%c) on line %zu\n",
                  line[i], current_line);
          }
          break;

          // comp -> jump
        case ';':
          // if the current value is dest (we are assuming it's dest at the
          // start) then the actual one is comp since we encountered a ;
          if (pointer_to_value_changed == dest->value) {
            strcpy(comp->value, dest->value);
            comp->validity = true;

            dest->validity = false;
            strcpy(dest->value, "");
          } else if (pointer_to_value_changed != comp->value) {
            cleanup(assembly_file, output_file, line, NULL, comp_hashmap,
                    jump_hashmap);
            error("SYNTAX ", "Unexepected character (%c) on line %zu\n",
                  line[i], current_line);
          }

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
          else {
            cleanup(assembly_file, output_file, line, NULL, comp_hashmap, jump_hashmap);
            error("SYNTAX ", "Unexepected character (%c) on line %zu\n",
                  line[i], current_line);
          }
          break;
        }

        // Unhandled character
      } else {
        cleanup(assembly_file, output_file, line, NULL, comp_hashmap, jump_hashmap);
        error("SYNTAX ", "Unexepected character (%c) on line %zu\n", line[i],
              current_line);
      }
    }

    // Break since we are in a comment
    if (parsing_status == SLASH_TWO)
      break;
  }

  // Make sure dest, jump and comp is set correctly
  // (if they are valid they MUST contain something)

  // make sure we are a c instruction
  if ((*instruction_parsed == C_INSTRUCTION) &&
      ((dest->validity == true && strcmp(dest->value, "") == 0) ||
       (jump->validity == true && strcmp(jump->value, "") == 0) ||
      (comp->validity == true && strcmp(comp->value, "") == 0)) {
    cleanup(assembly_file, output_file, line, NULL, comp_hashmap, jump_hashmap);
    error("SYNTAX ", "Unexepected character end of line %zu\n", current_line);
  }
}
