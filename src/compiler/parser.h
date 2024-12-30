#include "hashmap.h"
#include <stdbool.h>
#include <stdio.h>

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

struct c_instruction_value {
  char value[4];
  bool validity;
};

bool parse_line(char *line, enum instruction *instruction_parsed,
                char *a_or_label_value, struct c_instruction_value *dest,
                struct c_instruction_value *comp,
                struct c_instruction_value *jump);
