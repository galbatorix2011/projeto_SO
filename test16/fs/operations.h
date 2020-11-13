#ifndef FS_H
#define FS_H
#include "state.h"
#include "pthread_operations.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType, locked_stack * stack);
int delete(char *name, locked_stack * stack);
int lookup(char *name, locked_stack *stack, func_type f_type);
void print_tecnicofs_tree(char * output_file);

#endif /* FS_H */
