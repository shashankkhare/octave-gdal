// in case of error the function fill a value_list  
// with -1 
#include "misc.h"
void get_null_values(int args_num,octave_value_list *list) {
 for (int i=0;i<args_num;i++) (*list)(i) = octave_value(-1); 
}

