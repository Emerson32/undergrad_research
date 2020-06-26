#ifndef ID_LIST_H
#define ID_LIST_H

struct ID_List;

/*
*   Attempts to allocate memory for an ID_List structure.
*   Upon success this function returns a pointer to the structure,
*   otherwise it returns NULL
*/
struct ID_List *id_list_create();

/* 
*   Simply frees the memory allocated by id_list_create()
*/
void id_list_destroy(struct ID_List *list);

/*
*   Adds a bus ID to the ID_List and the list grows when necessary.
*   Upon success this function returns 0.
*   If this function fails to reallocate the necessary memory,
*   then -1 is returned and the original list remains untouched.
*/
void id_list_add(struct ID_List *list, char *id);

/*
*   Removes the bus IDs currently in the list
*/
void id_list_clear(struct ID_List *list);

/*
*   Returns the current size of the list
*/
int id_list_size(struct ID_List *list);

/*
* Retrieves a list element at the specified index.
* Upon success, this function returns a pointer to the element,
* otherwise NULL is returned.
*/
char *id_list_get(struct ID_List *list, int index);

#endif