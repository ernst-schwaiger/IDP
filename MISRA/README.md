# MISRA Rules Slide Deck

## Example Program

- sudo apt install make gcc cppcheck
- compile the program with `make`
- run cppcheck with `make check`

## Stefan

### Dir 4.11 
The validity of values passed to library functions shall be checked

### Dir 4.12
Dynamic memory allocation shall not be used

### Dir 4.13 
Functions which are designed to provide operations on a resource should be called in an appropriate sequence

### Rule 2.1 
A project shall not contain unreachable code

## Lorenzo

### Rule 2.2
There shall be no dead code

### Rule 2.3 
A project should not contain unused type declarations

### Rule 21.7 
The atof, atoi, atol and atoll functions of <stdlib.h> shall not be used

## Ernst

### Rule 2.4
>A project should not contain unused tag declarations.  

| Category  | Analysis | Applies to |
| -         | -        | -          |
|  Required | Decidable, System | C90, C99 |

>Rationale: If a tag is declared but not used, then it is unclear to a reviewer if the tag is redundant or it has been 
left unused by mistake.

Type tags follow after the `struct`, `enum`, and `union`, keywords, they form a separate namespace.  
Source: https://www.gnu.org/software/c-intro-and-ref/manual/html_node/Type-Tags.html  
Instances can be created by using that tag as well, e.g. 

```c
struct my_struct_tag { int a; float b; };
struct my_struct_tag my_struct_value = { 42, 3.14f };

enum my_enum_tag { foo = 0, bar, baz };
enum my_enum_tag my_enum_value = bar;

union my_union_tag { int variant_int; float variant_float; };
union my_union_tag my_union_value = { 42 };
```

Today, the usage of typedefs is more common:
```c
typedef struct { int a; float b; } my_struct_type;
my_struct_type my_struct_value = { 42, 3.14f };

typedef enum { foo = 0, bar, baz } my_enum_type;
my_enum_type my_enum_value = bar;

typedef union { int variant_int; float variant_float; } my_union_type;
my_union_type my_union_value = { 42 };
```

The only use for tags nowadays (I know of) are "incomplete types":

```c
typedef struct node_tag
{
    int value;
    struct node_tag *next; /* we can only use node_tag here, node_t is not yet available */
} node_t;
```

Code that does **not** comply with MISRA (provided `record_t` is not used in the code, only `record1_t`):
```c
typedef struct record_t                    /* Non-compliant */
{
  uint16_t  key; 
  uint16_t  val;
} record1_t;
```

MISRA compliant code:
```c
typedef struct
{
  uint16_t  key; 
  uint16_t  val;
} record1_t;
```


### Rule 21.8
>The library functions abort, exit, getenv and system of <stdlib.h> shall not be used  

| Category  | Analysis | Applies to |
| -         | -        | -          |
|  Required | Decidable, Single TU | C90, C99 |

>Amplification: The identifiers `abort`, `exit`, `getenv` and `system` shall not be used and no macro with one of these names shall be expanded.  
>Rationale: These functions have undeﬁned and implementation-deﬁned behaviours associated with them.

`void abort(void);`
Functions passed to atexit() are not called. Whether open resources such as files are closed is implementation defined. An implementation defined status is returned to the host environment that indicates unsuccessful execution.  
Source: https://en.cppreference.com/w/c/program/abort

`void exit( int exit_code );` takes an `exit_code` as parameter. On Unix-like systems a value of `0` indicates success of the terminating process, whereas any other value indicates failure. Other operating systems may use different values.
Source: https://en.cppreference.com/w/c/program/exit

`char *getenv( const char *name );` takes the name of an environment variable and returns a pointer to its value, or `null`. 

The set of environmental variables and methods of altering it are implementation-defined.  
Source: https://en.cppreference.com/w/c/program/getenv

The return value of `int system( const char *command );` is implementation defined.  
Source: https://en.cppreference.com/w/c/program/system

### Rule 21.9 
>The library functions `bsearch` and `qsort` of <stdlib.h> shall not be used 

| Category  | Analysis | Applies to |
| -         | -        | -          |
|  Required | Decidable, Single TU | C90, C99 |

>Amplification: The identifiers bsearch and qsort shall not be used and no macro with one of these names shall be expanded.  
>Rationale: If the comparison function does not behave consistently when comparing elements, or it modiﬁes any of the elements, the behaviour is undeﬁned.  

```c
void qsort( void* ptr, size_t count, size_t size, int (*comp)(const void*, const void*) );
void* bsearch( const void *key, const void *ptr, size_t count, size_t size, int (*comp)(const void*, const void*) );
```
Both functions are passed a comparison function `comp` as parameter. That function shall return a value `< 0` if the first value is less than the second one, a value of `0` if both elements are equal, or a value `> 0` if the first element is greater than the second one.

If two elements exist in the array passed to qsort are equal, their relative order is implementation defined. If `key` exists more than once in the array passed to `bsearch` it is implementation dependent, which of the elements with the proper key is actually returned.

Moreover, `qsort` might be implemented as a recursive function, hence it must not be used in environments that forbid recursive functions in general.

### Rule 21.10
>The Standard Library time and date functions shall not be used  

| Category  | Analysis | Applies to |
| -         | -        | -          |
|  Required | Decidable, Single TU | C90, C99 |

>Amplification: None of the facilities that are speciﬁed as being provided by <time.h> shall be used.  
>Amplification: In C99, the identifier `wcsftime` shall not be used and no macro with this name shall be expanded.  
>Rationale: The time and date functions have unspeciﬁed, undeﬁned and implementation-deﬁned behaviours associated with them.

`time_t time( time_t* arg );` Returns the current calendar time encoded as a time_t object, and also stores it in the time_t object pointed to by arg (unless arg is a null pointer).  
The encoding of calendar time in time_t is unspecified, but most systems conform to POSIX specification and return a value of integral type holding the number of seconds since the Epoch. Implementations in which time_t is a 32-bit signed integer (many historical implementations) fail in the year 2038.  
Source: https://en.cppreference.com/w/c/chrono/time.html
