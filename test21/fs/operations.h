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
int move(char * path1, char* path2, locked_stack * stack);
int create_move_locks(locked_stack *stack, int size1, char* parent_name1, int size2, char* parent_name2, int * parent_inumber1, int *parent_inumber2);
int lookup_successful(locked_stack * stack, int parent_inumber1, int parent_inumber2);
void lookup_lock_inode(locked_stack * stack, char * path, int inumber, func_type f_type);


#endif /* FS_H */
