#include <stdlib.h>
#include <string.h>

#define DEFAULT_CAPACITY 4

struct ID_List{
    int size;
    int capacity;
    char **data;
} list;

struct ID_List *id_list_create()
{
    struct ID_List *list = malloc(sizeof(struct ID_List));
    if (list == NULL)
    {
        return NULL;
    }

    list->size = 0;
    list->capacity = DEFAULT_CAPACITY;
    list->data = calloc(DEFAULT_CAPACITY, sizeof(char *));
    if (list->data == NULL)
    {
        free(list);
        return NULL;
    }

    return list;
}

void id_list_destroy(struct ID_List *list)
{
    free(list);
}

int id_list_add(struct ID_List *list, char *id)
{
    if (list->size == list->capacity)
    {
        int new_capacity = list->capacity * 2;
        char ** new_data = realloc(list->data, new_capacity * sizeof(char *));

        if (new_data == NULL)
            return -1;
        
        strncpy(new_data[list->size], id, strlen(id));
        // new_data[list->size] = id;

        list->data = new_data;
        list->capacity = new_capacity;
    }
    else
    {
        strncpy(list->data[list->size], id, strlen(id));
        // list->data[list->size] = id;
    }
    list->size += 1;

    return 0;
}

void id_list_clear(struct ID_List *list)
{
    list->size = 0;
    free(list->data);
    list->data = NULL;
}

int id_list_size(struct ID_List *list)
{
    return list->size;
}

int id_list_capacity(struct ID_List *list)
{
    return list->capacity;
}
char *id_list_get(struct ID_List *list, int index)
{
    if (index >= list->size || index < 0)
    {
        return NULL;
    }
    return list->data[index];
}