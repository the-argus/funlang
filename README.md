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
- built-in vector types with swizzling
- odin/jai struct `using` statement
- an odin-style context pointer
- shadowing
- the `static struct` for easily switching between something being a struct
  you have to instantiate vs. a singleton. just switch `struct` and
  `static struct`
- python style fstrings (which use the context's arena allocator by default)
- `defer`
- functions can be inside of a struct, in which case they can use the `self`
  keyword for their first arguments to become memeber methods
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
*u8 bytes = some_function();
```

The equivalent C code:

```c
const uint8_t* const bytes = some_function();
```

A declaration in funlang:
(the first `var` modifies `*` and the second `var` modifies the `u8`)

```rs
var *var u8 bytes = some_function();
```

The equivalent C code:

```c
uint8_t* bytes = some_function();
```

A declaration in funlang:

```rs
*var u8 bytes = some_function();
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
  and enum declarations, which are signalled by `++ struct` and
  `++ enum`, respectively. Though both structs and enums are also types.
- A function is signalled by `++ fun`
- A compile time known variable is signalled by `++ <known typename> ...`
- All of the above can be preceded by a `--` instead of `++`, to create a
  private declaration. A private declaration is visible only within the scope
  it was declared in and inner scopes.

```rs
-- type std = @import("std");

++ i32 i;

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

### Struct syntax, and procedural vs. struct scope

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

The scope of a file is also the scope of a struct. For this reason it is not
possible to create static variables which are accessible from functions in the
same scope. To access variables declared at file scope, use `self` or `*self` or
`*var self`:

```rs
++ type std = @import("std");

i32 myint = 32;

++ fun test (*var self) {
    std::print("f{self.myint}");
    self.myint += 1;
};
```

Procedural scope may include expressions which are not assigned. All expressions
and assignments are calls to other procedures (`fun`s) or primitive operations
such as math, copying, or taking the address of something. These expressions
will have side effects and will be run in consecutive order at runtime when the
`fun` is called. There is a reserved `fun` name, `main`, which is a `fun` which
is the entrypoint of the program.

#### `using` keyword

### Struct member access vs. namespace access

Struct members are accessed with the typical `object.member` syntax. This
includes methods and fields. For example:

```rs
-- type std = @import("std");

++ struct Person {
    *char name;
    u64 age;

    ++ fun printAge(self) {
        std::print(f"{self.age}");
    }
}

++ fun main() {
    Person p = { .name = "Guy", .age = 42 };
    p.printAge();
}
```

But if you are accessing members which are not specific to particular instances
of the struct, you use `::` instead of `.`. For example:

```rs
-- type std = @import("std");

-- struct my_functions {
    ++ fun printHello() {
        std::print("hello");
    }
    ++ fun printGoodbye() {
        std::print("hello");
    }

    ++ u64 size = 64;
};

++ func main() -> i32 {
    my_functions::printHello();
    my_functions::printGoodbye();
    return my_functions::size.to<i32>();
}
```
