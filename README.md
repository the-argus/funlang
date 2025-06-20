# funlang

A compiled programming language, for fun

## Features and examples

We're gonna have:

- C-style variable declaration, except for const by default
- Zig style options
- Zig style non-nullable non-arithmetic-able pointers
- Results, but the "error" value is always present and is instead called a status.
- Rust-style enums (tagged unions, basically)
- Match statements with the ability to match on tagged unions
- Zig style captures for `if`, `while`, and `for`
- bounds checked slices
- defined overflow behavior
- odin/jai struct `using` statement
- an odin-style context pointer
- python style fstrings (which use the context's arena allocator by default)
- `defer`
- universal function call syntax + method-style function call syntax
- functions can be in the namespace of a struct
- instead of labeled blocks, there are "taggable" blocks. mark a block `'` or
  `''` and then do `break';` `break'';` etc
- A few built-in traits, no support for user-defined traits:
  - Iter
  - RandomAccessIter
  - Destroy
  - Clone
  - Default
  - Status
- No metaprogramming facilities, except for `if comptime` for conditional
  compilation. No templates or generic types besides the built-in ones
- Some nice built-in types:
  - ArraySet\<T\>
  - SegmentedSet\<T\>
  - ArrayList\<T\>
  - SegmentedList\<T\>
  - LinkedList\<T\> (accepts a size hint for allocating blocks of items together)
  - DoublyLinkedList\<T\>
  - ArrayHashMap<K, V>
  - SegmentedHashMap<K, V>
  - ArrayPool\<T\> (wrapper over array pool allocator)
  - SegmentedPool\<T\> (wrapper over segmented pool allocator)
  - GenerationalArrayPool\<T, I, G\> (contained type, index integer type, generation integer type)
  - GenerationalSegmentedPool\<T, I, G\>
  - ArrayArena\<T\> (wrapper over array arena allocator)
  - SegmentedArena\<T\> (wrapper over segmented arena allocator)
  - GraphNode\<T\, Container> (item T and either ArrayList or SegmentedList of connected nodes, doubly linked)
  - PtrGraphNode\<T\, Container> (item T and either ArrayList or SegmentedList of *pointers* to connected nodes, doubly linked)
  - Tree\<T\> (stores its children in an ArrayList)
  - PtrTree\<T\> (stores an ArrayList of pointers to its children)
  - BinaryTree\<T\>
  - BitArray
  - BitArrayList

### Variable declaration syntax and const-ness

A declaration in funlang:

```rs
i32 i = 0;
```

The equivalent C code:

```c
const int32_t i = 0;
```

A declaration in funlang:

```rs
u8* bytes = some_function();
```

The equivalent C code:

```c
const uint8_t* const bytes = some_function();
```

A declaration in funlang:
(the first `var` modifies `u8` and the second `var` modifies `*`. Rather than
reading this as "a pointer to var" or reading a `T*` as a "pointer to const,"
you can read it as a "const pointer" or "var pointer").

```rs
var u8 var* bytes = some_function();
```

The equivalent C code:

```c
uint8_t* bytes = some_function();
```

A declaration in funlang:

```rs
u8 var* bytes = some_function();
```

The equivalent C code:

```c
uint8_t* const bytes = some_function();
```

### Uninitialized variables

All variables must be initialized with an expression. The following is a compile
error:

```rs
var i32 myvar;
```

### Declarations

Declarations (of structs, type aliases, compile time known variables, and
functions) are preceded by a `++` or a `--` (public and private, respectively).

- A type declaration is signalled by `++ type`, with the exception of struct
  and namespace declarations, which are signalled by `++ struct` and
  `++ namespace`, respectively. Though both structs and namespaces are types.
- A function is signalled by `++ fun`
- A compile time known variable is signalled by `++ <known typename> ...`

```rs
// a file is namespace scope
-- type std = @import("std");

++ struct Example
{
    i32 i;
    f32 f;
};

-- type MyAlias = Example;

++ fun main()
{
    ++ comptime u64 max_strings = 20;

    var std::Arena arena = std::Arena::nogrow(std::c_allocator.alloc(1024 * 1024)));
    var std::StringList strlist = std::StringList::new(arena ref);
    u8[] args = std::args_alloc(arena);

    for args, 0.. => arg, idx {
        if idx >= max_strings {
            break;
        }
        if is_flag(arg) { // definition not shown
            std::print(f"got flag at {idx}");
        }
    }
};
```

### Struct syntax, and procedural vs. struct vs. namespace scope

Struct definitions are a type of declaration so they are preceded by a `++`.
`++ struct <Name>` precedes a block (a set of curly braces). Inside this block
code is in *struct scope*. Variable declarations define what fields will be
present in the struct when it instantiated. Initialization describes *default
values* for the fields of the struct, which can be overwritten by the programmer
when instantiating the struct. Providing no initialization value is allowed in
struct scope, as it means that that field will have to be initialized at
struct initialization time. It is valid to introduce other declarations within
struct scope, including nesting another struct declaration.

```rs
++ struct MyCustomArray
{
    u8*? bytes;
    u64*? generations;
    u64 size;
    u64 len;

    ++ type Index = i32;

    ++ struct Handle
    {
        Index idx;
        u64 generation;
    };
};
```

The namespace scope is identical to struct scope syntactically, except that
variable declarations are declaring variables in the data segment of the
program which will be accessible for its entire duration. Initialization
expressions must be provided for all variables, the same as procedural scope.

```rs
// namespace scope
++ type std = @import("std");

var i32 my_global_int = 32;

++ fun test () {
    std::print("f{my_global_int}");
    my_global_int += 1;
};
```

Procedural scope may include expressions which are not assigned. All expressions
and assignments are calls to other procedures (`fun`s) or primitive operations
such as math, copying, or taking the address of something. These expressions
will have side effects and will be run in consecutive order at runtime when the
`fun` is called. There is a reserved `fun` name, `main`, which is a `fun` which
is the entrypoint of the program.

#### `using` keyword

## Struct member access and namespace access
