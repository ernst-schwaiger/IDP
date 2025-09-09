#include <stdio.h>

/* Types */

typedef struct node_tag
{
    int value;
    struct node_tag *next;
} node_t;

/* Global Variables */

typedef struct { int a; float b; } my_struct_type;
my_struct_type my_struct_value = { 42, 3.14f };

typedef enum { foo = 0, bar, baz } my_enum_type;
my_enum_type my_enum_value = bar;

typedef union { int variant_int; float variant_float; } my_union_type;
my_union_type my_union_value = { 42 };


/* Function forward declarations */

void test_linked_list( void );

int main(int argc, char *argv[])
{
    test_linked_list();
}


void test_linked_list( void )
{
    node_t node4 = { 42, NULL };
    node_t node3 = { 41, &node4 };
    node_t node2 = { 40, &node3 };
    node_t node1 = { 39, &node2 };

    node_t *curr_node = &node1;
    while (curr_node != NULL)
    {
        (void)printf("Node value: %d\n", curr_node->value);
        curr_node = curr_node->next;
    }
}

