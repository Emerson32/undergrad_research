#include <stdlib.h>

#include "dbg.h"
#include "id_list.h"

static void test_init()
{
    struct ID_List *list = id_list_create();
    check(list, "Failed to initialize the list");
    
    check((id_list_size(list) == 0), "Incorrect size upon init");
    
    error:
    id_list_destroy(list);

    return;
}

static void test_add()
{
    struct ID_List *list = id_list_create();
    check(list, "Failed to initialize the list");

    char *test_id1 = "1-1.2.3:1.0";
    char *test_id2 = "1-1.2.4:1.0";

    id_list_add(list, test_id1);
    id_list_add(list, test_id2);

    check(
        strcmp(id_list_get(list, 0), test_id1) == 0,
        "Test_id1 was incorrectly stored"
    );
    
    check(
        strcmp(id_list_get(list, 1), test_id2) == 0,
        "Test_id2 was incorrectly stored"
    );

    check(id_list_size(list) == 2, "Size mismatch");

    error:
    id_list_destroy(list);
    return;
}

static void test_growth()
{
    struct ID_List *list = id_list_create();
    check(list, "Failed to initialize the list");

    char *test_ids[5] = {
        "1-1.1.1:1.0",
        "1-1.1.2:2.0",
        "1-1.1.3:3.0",
        "1-1.1.4:4.0",
        "1-1.1.5:5.0",
    };

    for (int i = 0; i <= 4; i++)
    {
        int res;

        res = id_list_add(list, test_ids[i]);

        // Allocation failed, so retry
        while (res < 0)
        {
            res = id_list_add(list, test_ids[i]);
        }
    }

    check(
        (id_list_size(list) == 5),
        "Insertion after growth failed"
    );
    
    check(
        id_list_capacity(list) == 8,
        "Growth capacity is incorrect"
    );

    for (int i = 0; i <= 4; i++)
    {
        check(
            strcmp(id_list_get(list, i), test_ids[i]) == 0,
            "ID insertion during growth failed at index %d", i
        );
    }

    error:
    id_list_destroy(list);
    return;
}

static void test_clear()
{
    struct ID_List *list = id_list_create();
    

    char *test_ids[5] = {
        "1-1.1.1:1.0",
        "1-1.1.2:2.0",
        "1-1.1.3:3.0",
        "1-1.1.4:4.0",
        "1-1.1.5:5.0",
    };

    for (int i = 0; i <= 4; i++)
    {
        int res;

        res = id_list_add(list, test_ids[i]);

        // Allocation failed, so retry
        while (res < 0)
        {
            res = id_list_add(list, test_ids[i]);
        }
    }

    id_list_clear(list);

    check(
        id_list_size(list) == 0,
        "List size not reset after clear"
    );

    check(
        id_list_capacity(list) == 8,
        "List capacity attribute illegally modified"
    );

    for (int i = 0; i <= 7; i++)
    {
        check(
            !(id_list_get(list, i)),
            "Clear operation failed"
        );
    }
    
    error:
    id_list_destroy(list);

    return;
}

static void all_tests()
{
    test_init();
    test_add();
    test_growth();
    test_clear();
}

int main()
{
    all_tests();
}