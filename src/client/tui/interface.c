#define _XOPEN_SOURCE_EXTENDED
#include "interface.h"
#include "render.c"		// Not strictly needed to include .c if we compile them separately, but maybe for keeping same structure if not updating compile.sh immediately. But I WILL update compile.sh.
// Actually, I will make this file empty or just includes, and REMOVE it from
// compile.sh, OR I will include it and have it empty. Since I promised to
// update compile.sh, I will remove interface.c from compile.sh. But I will
// write to it to make it empty so I don't leave dead code if I forget.
// Actually, I should probably delete the content of this file to avoid
// confusion.
