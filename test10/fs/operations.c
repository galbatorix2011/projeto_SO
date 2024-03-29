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


/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name, locked_stack *stack, func_type f_type) {
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";

	strcpy(full_path, name);

	char *path = strtok(full_path, delim);

	if (path == NULL && f_type == F_WRITE)
		latch_lock(FS_ROOT, L_WRITE);
	else
		latch_lock(FS_ROOT, L_READ);
	queue_locked_stack(stack, FS_ROOT);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	/* get root inode data */
	inode_get(current_inumber, &nType, &data);


	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok(NULL, delim);
		if (path == NULL && f_type == F_WRITE)
			latch_lock(current_inumber, L_WRITE);
		else
			latch_lock(current_inumber, L_READ);
		queue_locked_stack(stack, current_inumber);
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


