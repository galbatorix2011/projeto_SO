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
int create_move_locks(locked_stack *stack, int size1, char* parent_name1, int size2, char* parent_name2, 
                            int * parent_inumber1, int *parent_inumber2, char * path1, char *path2);
int aux_move_origin(locked_stack *stack, char* parent_name1, 
				int * parent_inumber1, char * path1, char *path2, func_type f_type,  char* child_name1, char *child_name2, int * child_inumber1, int same_path);
int aux_move_destiny(locked_stack *stack, char* parent_name2, 
				int * parent_inumber2, char * path1, char *path2, char *child_name2, func_type f_type);
int lookup_lock_inode(locked_stack * stack, char * path, int inumber, func_type f_type, int * move_dif_stop_checking);
int check_invalid_subDir(char* child_name2, char* parent_name2);


#endif /* FS_H */
