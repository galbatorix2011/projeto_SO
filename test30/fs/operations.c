#include "operations.h"
#include "pthread_operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define FAIL_TO_CREATE_LOCKS_MOVE -2
#define MAX_TIME 1
#define MAY_BE_SUBDIR 3


/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {
	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);
	
	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;
}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();
	
	/* create root inode */
	int root = inode_create(T_DIRECTORY);
	
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
	if (entries == NULL) {

		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
            return entries[i].inumber;
        }
    }
	return FAIL;
}


/*
 * Locks the the origin path and also checks if it checks all the requirements
 * Inputs:
 *   - f_type: F_WRITE (if its the first path being locked) or F_MOV_SECOND (otherwise) 
 *   - same_path: if both parent names have the same path, only one lock ahs to be done
 * Output:
 *   - returns SUCCESS, FAIL or MAY_BE_SUBDIR
 */ 
int aux_move_origin(locked_stack *stack, char* parent_name1, int * parent_inumber1, char * path1, char *path2, 
			func_type f_type, char* child_name1, char *child_name2, int * child_inumber1, int same_path) {

	type pType1;
	union Data pdata1;

	if ((*parent_inumber1 = lookup(parent_name1, stack, f_type)) == FAIL) {
		unlock_locked_stack(stack);
		printf("failed to move %s to %s, invalid origin parent dir %s\n",
		        path1, path2, parent_name1);
		return FAIL;
	}

	inode_get(*parent_inumber1, &pType1, &pdata1);

	if(pType1 != T_DIRECTORY) {
		printf("failed to move %s to %s, parent origin %s is not a dir\n", path1, path2, parent_name1);
		return FAIL;
	}
	
	// the move can only be done if the origin parent has the origin child
	if ((*child_inumber1 = lookup_sub_node(child_name1, pdata1.dirEntries)) == FAIL) {
		printf("failed to move %s to %s, %s doesnt exist in dir %s\n",
		       path1, path2, child_name1, parent_name1);
		return FAIL;
	}

	// in this case, the move will act as a rename, and so we need to check if the destiny child
	// exists in the parent name
	if (same_path && lookup_sub_node(child_name2, pdata1.dirEntries) != FAIL) {
		printf("failed to move %s to %s, %s already exists in destiny dir %s\n",
		       path1, path2, child_name2, parent_name1);
		return FAIL;
	}

	// if its the first path being lock and its child is a dir we can be in a situation such as this:
	// "m a/b a/b/c/x". this shouldn't be possible so we return MAY_BE_SUBDIR so we can check 
	//if thats the case or not
	if (f_type == F_WRITE && get_inode_type(*child_inumber1) == T_DIRECTORY)
		return MAY_BE_SUBDIR;

	return SUCCESS; 		
}

/*
 * Locks the the destiny path and also checks if it checks all the requirements
 * Output:
 *   - returns SUCCESS or FAIL 
 */ 
int aux_move_destiny(locked_stack *stack, char* parent_name2, 
				int * parent_inumber2, char * path1, char *path2, char *child_name2, func_type f_type) {
	type pType2;
	union Data pdata2;

	if ((*parent_inumber2 = lookup(parent_name2, stack, f_type)) == FAIL) {
		unlock_locked_stack(stack);
		printf("failed to move %s to %s, invalid destiny parent dir %s\n",
		        path1, path2, parent_name2);
		return FAIL;
	}

	inode_get(*parent_inumber2, &pType2, &pdata2);

	if(pType2 != T_DIRECTORY) {
		printf("failed to move %s to %s, parent destiny %s is not a dir\n", path1, path2, parent_name2);
		return FAIL;
	}
	 
	if (lookup_sub_node(child_name2, pdata2.dirEntries) != FAIL) {
		printf("failed to move %s to %s, %s already exists in destiny dir %s\n",
		       path1, path2, child_name2, parent_name2);
		return FAIL;
	}

	return SUCCESS; 		
}

/*
 * checks the case "m a/b a/b/c/x" (if b is a dir) by checking if a "b" is in "a/b/c/x"
 * Output:
 *   - returns SUCCESS or FAIL 
 */ 
int check_invalid_subDir(char* child_name2, char* parent_name2){
	char *saveptr;
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";

	strcpy(full_path, parent_name2);
	char *path = strtok_r(full_path, delim, &saveptr); 
	while (path != NULL) {
		if (strcmp(child_name2, path) == 0)
			return FAIL;
		path = strtok_r(NULL, delim, &saveptr);
	}
	return SUCCESS;
}

/*
 * Moves a file or dir from the origin path to the destiny path
 * Input:
 * 	 - path1: origin path
 *   - path2: destiny path
 *   - stack: stack that will have the inumbers of the locked nodes
 * Output:
 *   - returns SUCCESS or FAIL 
 */ 
int move(char *path1, char* path2, locked_stack* stack) {
	int parent_inumber1, parent_inumber2;
	int child_inumber1;
	
	int str_cmp;

	char *parent_name1, *child_name1, name_copy1[MAX_FILE_NAME];
	char *parent_name2, *child_name2, name_copy2[MAX_FILE_NAME];   

	strcpy(name_copy1, path1);
	strcpy(name_copy2, path2);

	split_parent_child_from_path(name_copy1, &parent_name1, &child_name1);
	split_parent_child_from_path(name_copy2, &parent_name2, &child_name2);

	str_cmp = strcmp(parent_name1, parent_name2);

	if (str_cmp < 0){
		int test;
		if ((test = aux_move_origin(stack, parent_name1, &parent_inumber1, path1, 
				path2, F_WRITE, child_name1, child_name2, &child_inumber1, 0)) == FAIL) {
			unlock_locked_stack(stack);
			return FAIL;
		}
		
		if (test == MAY_BE_SUBDIR){
			if (check_invalid_subDir(child_name1, parent_name2) == FAIL){
				printf("failed to move %s to %s, origin child %s is a subDir of the parent destiny %s\n",
		    	path1, path2, child_name2, parent_name2);
				unlock_locked_stack(stack);
				return FAIL;
			}
		}
		
		if (aux_move_destiny(stack, parent_name2, &parent_inumber2, path1,
				path2, child_name2, F_MOV_SECOND) == FAIL){
			unlock_locked_stack(stack);
			return FAIL;
		}
	}

	else if (str_cmp > 0){

		if (aux_move_destiny(stack, parent_name2, &parent_inumber2, path1, 
				path2, child_name2, F_WRITE) == FAIL){
			unlock_locked_stack(stack);
			return FAIL;
		}

		if (aux_move_origin(stack, parent_name1, &parent_inumber1, path1, 
				path2, F_MOV_SECOND, child_name1, child_name2, &child_inumber1, 0) == FAIL) {
			unlock_locked_stack(stack);
			return FAIL;
		}
	}

	else {
		// if the parents' paths are the same, we only need to do the lookup once,
		// as parent_inumber1 = parent_inumber2
		if (aux_move_origin(stack, parent_name1, &parent_inumber1, path1, 
				path2, F_WRITE, child_name1, child_name2, &child_inumber1, 1) == FAIL) {
			unlock_locked_stack(stack);
			return FAIL;
		}
		parent_inumber2 = parent_inumber1;
	}
	
	if (dir_reset_entry(parent_inumber1, child_inumber1) == FAIL) {
		unlock_locked_stack(stack);
		printf("failed to delete %s from origin dir %s\n",
		       child_name1, parent_name1);
		return FAIL;
	}

	if (dir_add_entry(parent_inumber2, child_inumber1, child_name2) == FAIL) {
		unlock_locked_stack(stack);
		printf("could not add entry %s in destiny dir %s\n",
		       child_name2, parent_name2);
		return FAIL;
	}
	
	unlock_locked_stack(stack);
	return SUCCESS;
}



/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType, locked_stack * stack){
	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name, stack, F_WRITE);

	if (parent_inumber == FAIL) {
		unlock_locked_stack(stack);
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		unlock_locked_stack(stack);
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
		return FAIL;
	}

	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		unlock_locked_stack(stack);
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType);
	
	if (child_inumber == FAIL) {
		unlock_locked_stack(stack);
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name);
		return FAIL;
	}

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		unlock_locked_stack(stack);
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		return FAIL;
	}
	unlock_locked_stack(stack);
	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name, locked_stack * stack){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);


	parent_inumber = lookup(parent_name, stack, F_WRITE);

	if (parent_inumber == FAIL) {
		unlock_locked_stack(stack);
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		unlock_locked_stack(stack);
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	if (child_inumber == FAIL) {
		unlock_locked_stack(stack);		
		printf("could not delete %s, does not exist in dir %s\n", name, parent_name);
		return FAIL;
	}

	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		unlock_locked_stack(stack);
		printf("could not delete %s: is a directory and not empty\n", name);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		unlock_locked_stack(stack);
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		unlock_locked_stack(stack);
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
		return FAIL;
	}

	unlock_locked_stack(stack);
	return SUCCESS;
}


/*
 * Locks the next inode of the path if it is the last inode
 */
void lock_NULL_path(locked_stack * stack, int inumber, func_type f_type){
	
	// if the type is F_WRITE or F_MOV_SECOND than its means we are going to change that dir
	// and therefore have to do a write lock
	if (f_type == F_WRITE || f_type == F_MOV_SECOND){
		lock_inode(inumber, L_WRITE);
	}
	// if we will only be reading the path, then we can do a read lock instead
	if (f_type == F_READ)
		lock_inode(inumber, L_READ);
	push_locked_stack(stack, inumber);
}

void lock_not_NULL_path(locked_stack * stack, int inumber, func_type f_type, int *move_stop_checking){

	// if the type is F_WRITE, F_READ, F_MOV_SECOND(and we dont have to check the the node has already beem lock)
	// we can simply lock the node for reading
	if (f_type == F_READ || f_type == F_WRITE || (f_type == F_MOV_SECOND && *move_stop_checking)){
		lock_inode(inumber, L_READ);
		push_locked_stack(stack, inumber);
	}

	// if the type is F_MOVE_SECOND, and we have  to check and the inumbe isn't locked, we can lock it
	// for reading and cheange the value of move_stop_checking for 1
	else if (f_type == F_MOV_SECOND && !(*move_stop_checking) && !is_inumber_locked(stack, inumber)){
		lock_inode(inumber, L_READ);
		push_locked_stack(stack, inumber);
		*move_stop_checking = 1;
	}
}

/* 
 * Locks the inode in a different way, whether its the last inode of a path or not.
 * if it is, than the path will be NULL;	
 */
void lookup_lock_inode(locked_stack * stack, char * path, int inumber, func_type f_type, int * move_stop_checking){
	if (path)
		lock_not_NULL_path(stack, inumber, f_type, move_stop_checking);
	else if (!path)
		lock_NULL_path(stack, inumber, f_type);
}

/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name, locked_stack *stack, func_type f_type) {
	char *saveptr;
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";

	/* 
	 * once the path of the second path of the move function diverges from the first one
	 * we don't have to check for each node if it has already been locked. When that happens
	 * the value of move_stop_checking is switched to 1
	 */
	int move_stop_checking = 0;

	strcpy(full_path, name);

	char *path = strtok_r(full_path, delim, &saveptr); 

	lookup_lock_inode(stack, path, FS_ROOT, f_type, &move_stop_checking);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	/* get root inode data */
	inode_get(current_inumber, &nType, &data);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok_r(NULL, delim, &saveptr);
		lookup_lock_inode(stack, path, current_inumber, f_type, &move_stop_checking);
		inode_get(current_inumber, &nType, &data);
	}
	return current_inumber;
}



/*
 * Prints tecnicofs tree.
 * Input:
 *  - output_file: path to the output file
 */
void print_tecnicofs_tree(char * output_file){
	FILE *fp;
    fp = fopen(output_file,"w");
	
	if (fp == NULL){
        printf("Error: could not open the output file\n");
        exit(EXIT_FAILURE);
    }

	inode_print_tree(fp, FS_ROOT, "");

	if (fclose(fp) == EOF){
		printf("Error: could not close the output file\n");
        exit(EXIT_FAILURE);
	}
}


