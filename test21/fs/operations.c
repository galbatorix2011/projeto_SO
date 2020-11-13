#include "operations.h"
#include "pthread_operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
int split_parent_child_from_path(char * path, char ** parent, char ** child) {
	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);
	int size = 0;
	
	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			size++;
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return size;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;
	return size;

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

int lookup_successful(locked_stack * stack, int parent_inumber1, int parent_inumber2){
	if (parent_inumber1 == FAIL || parent_inumber2 == FAIL) {
		printf("failed to move, invalid directory\n");
		return FAIL;
	}
	return SUCCESS;
}

int create_move_locks(locked_stack *stack, int size1, char* parent_name1, int size2, char* parent_name2, 
			int * parent_inumber1, int *parent_inumber2) {
	if (size1 != size2) {

		if (size1 < size2){
			*parent_inumber1 = lookup(parent_name1, stack, F_MOV_DIF_FIRST_O);
			*parent_inumber2 = lookup(parent_name2, stack, F_MOV_DIF_SECOND_D);
		}

		else {
			*parent_inumber2 = lookup(parent_name2, stack, F_MOV_DIF_FIRST_D);
			*parent_inumber1 = lookup(parent_name1, stack, F_MOV_DIF_SECOND_O);
		}

		if (lookup_successful(stack, *parent_inumber1, *parent_inumber2) == FAIL){
			return FAIL;
		}
	}

	else {
		*parent_inumber1 = lookup(parent_name1, stack, F_MOV_SAME_FIRST_O);
		*parent_inumber2 = lookup(parent_name2, stack, F_MOV_SAME_SECOND_D);


		if (lookup_successful(stack, *parent_inumber1, *parent_inumber2) == FAIL){
			return FAIL;
		}

		if (*parent_inumber1 < *parent_inumber2){
			latch_lock(*parent_inumber1, L_WRITE);
			queue_locked_stack(stack, *parent_inumber1);
			latch_lock(*parent_inumber2, L_WRITE);
			queue_locked_stack(stack, *parent_inumber2);
			
		}

		else if (*parent_inumber1 > *parent_inumber2){
			latch_lock(*parent_inumber2, L_WRITE);
			queue_locked_stack(stack, *parent_inumber2);
			latch_lock(*parent_inumber1, L_WRITE);
			queue_locked_stack(stack, *parent_inumber1);
		}

		else{
			latch_lock(*parent_inumber2, L_WRITE);
			queue_locked_stack(stack, *parent_inumber2);
		}
	}

	return SUCCESS;
}

int move(char *path1, char* path2, locked_stack* stack) {
	printf("MOVEEEEEE\n");
	int parent_inumber1, parent_inumber2;
	int child_inumber1;
	type pType1, pType2;
	union Data pdata1, pdata2;
	int size1, size2;
	char *parent_name1, *child_name1, name_copy1[MAX_FILE_NAME];
	char *parent_name2, *child_name2, name_copy2[MAX_FILE_NAME];   
	strcpy(name_copy1, path1);
	strcpy(name_copy2, path2);
	size1 = split_parent_child_from_path(name_copy1, &parent_name1, &child_name1);
	size2 = split_parent_child_from_path(name_copy2, &parent_name2, &child_name2);
	printf("size 1 ---- %d\nsize2 ---- %d\n", size1, size2);
	if (create_move_locks(stack, size1, parent_name1, size2, parent_name2, &parent_inumber1, &parent_inumber2) == FAIL){
		printf("Could not move\n");
		unlock_locked_stack(stack);
		return FAIL;
	}
	
	inode_get(parent_inumber1, &pType1, &pdata1);

	if(pType1 != T_DIRECTORY) {
		print_stack(stack);
		unlock_locked_stack(stack);
		printf(" pai 1 n e dir    failed to move %s, parent %s is not a dir\n", path1, parent_name1);
		return FAIL;
	}
	 
	if ((child_inumber1 = lookup_sub_node(child_name1, pdata1.dirEntries)) == FAIL) {
		print_stack(stack);
		unlock_locked_stack(stack);
		printf(" filho n esta no pai    to move %s, doesnt exist in dir %s\n",
		       child_name1, parent_name1);
		return FAIL;
	}

	inode_get(parent_inumber2, &pType2, &pdata2);

	if(pType2 != T_DIRECTORY) {
		print_stack(stack);
		unlock_locked_stack(stack);
		printf(" pai 2 n e dir   failed to move %s, parent %s is not a dir\n", path2, parent_name1);
		return FAIL;
	}
	 
	if (lookup_sub_node(child_name2, pdata1.dirEntries) != FAIL) {
		print_stack(stack);
		unlock_locked_stack(stack);
		printf(" o filho ja ta no pai 2     failed to move %s, already exists in dir %s\n",
		       child_name2, parent_name2);
		return FAIL;
	}

	if (dir_reset_entry(parent_inumber1, child_inumber1) == FAIL) {
		print_stack(stack);
		unlock_locked_stack(stack);
		printf("n deu para dar rest to delete %s from dir %s\n",
		       child_name1, parent_name1);
		return FAIL;
	}
	//print_stack(stack);
	//unlock_locked_stack_move(stack);

	if (dir_add_entry(parent_inumber2, child_inumber1, child_name2) == FAIL) {
		print_stack(stack);
		unlock_locked_stack(stack);
		printf(" n deu para add not add entry %s in dir %s\n",
		       child_name1, parent_name2);
		return FAIL;
	}
	
	print_stack(stack);
	unlock_locked_stack(stack);
	return 0;
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

	/*
	* The funtion is locked from this point because once
	* the father is found, it cannot be destroyed until
	* its child is created
	*/

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

	/*
	* The funtion is locked from this point because once
	* the father is found, there cannot be a scenario where its child
	* and then the father is destroyed before the existence followed by
	* the delete of the child takes place
	*/


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

void lookup_lock_inode(locked_stack * stack, char * path, int inumber, func_type f_type){

	//if it is the last inode and the operation is for create(F_WRITE) then, it must do a write lock
	// if we are doing a lookup of two different paths then the last inode of each should have a write lock aswell
	if (path == NULL && (f_type == F_WRITE || f_type == F_MOV_DIF_FIRST_O || f_type == F_MOV_DIF_SECOND_O ||
				f_type == F_MOV_DIF_FIRST_D || f_type == F_MOV_DIF_SECOND_D)){
		if (f_type == F_WRITE)
			queue_locked_stack(stack, inumber);
		else
			queue_locked_stack_move(stack, inumber, f_type);
		latch_lock(inumber, L_WRITE);
			
	}

	// if we are doing the second lookup of mov, we must make sure we arent locking a locked inode
	else if (path && (f_type == F_READ || f_type == F_WRITE || f_type == F_MOV_DIF_SECOND_O || 
			f_type == F_MOV_SAME_SECOND_D || f_type == F_MOV_DIF_SECOND_D || f_type == F_MOV_SAME_SECOND_D)){
		locked_node * node;
		if (f_type == F_READ || f_type == F_WRITE || (node = is_inumber_locked(stack, inumber)) == NULL){
			if (f_type == F_READ || f_type == F_WRITE)
				queue_locked_stack(stack, inumber);
			else 
				queue_locked_stack_move(stack, inumber, f_type);
			latch_lock(inumber, L_READ);
			
		}
		else
			node->locked = 2;
			
	
	}
	// in this case we want to do a read lock if we wanna look for a node or if we are doing the first lookup of mov
	// and we havent go to the last inode yet
	else if (f_type == F_READ || (path && (f_type == F_MOV_DIF_FIRST_O || f_type == F_MOV_SAME_FIRST_O || 
				f_type == F_MOV_DIF_FIRST_D || f_type == F_MOV_SAME_FIRST_D))){
		if (f_type == F_READ)
			queue_locked_stack(stack, inumber);
		else
			queue_locked_stack_move(stack, inumber, f_type);
		latch_lock(inumber, L_READ);
	}

	//  if we are doing a mov with paths of the same size, we dont lock the last indodes in lookp, we lock them after it
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

	strcpy(full_path, name);

	char *path = strtok_r(full_path, delim, &saveptr); 

	lookup_lock_inode(stack, path, FS_ROOT, f_type);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	/* get root inode data */
	inode_get(current_inumber, &nType, &data);


	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		//printf("%s ---- ", path);
		path = strtok_r(NULL, delim, &saveptr);
		lookup_lock_inode(stack, path, current_inumber, f_type);
		inode_get(current_inumber, &nType, &data);
	}
	//printf("LAST INUMBER: %d\n", current_inumber);
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


