# funlang

A compiled programming language, for fun

## Features and examples

We're gonna have:

- C-style variable declaration, except for const by default
- Zig style options
- Zig style non-nullable non-arithmetic-able pointers
- Results, but the "error" value is always present and is instead called a status.
- Rust-style enums
- Match statements with the ability to match on enums
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
  keyword for their first arguments to become member methods
- instead of labeled blocks, there are "taggable" blocks. mark a block `'` or
  `''` and then do `break';` `break'';` etc
- A few built-in traits, no support for user-defined traits:
  - Iter
  - RandomAccessIter
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
  - TreeNode\<T\> (stores its children in an ArrayList)
  - PtrTreeNode\<T\> (stores an ArrayList of pointers to its children)
  - BinaryTreeNode\<T\>
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

### Ignored return values

The return value from every function must be used otherwise the compiler will
emit a warning. To silence the warning, assign the variable to the reserved
`_` match pattern:

```rs
_ = some_function_that_returns_something();
```

`_` is effectively a writeonly global variable which is always in scope and
always of the type you write to it. The black hole!

### Declarations

Declarations (of structs, type aliases, compile time known variables, public
static variables, and functions) are preceded by a `++` or a `--`. A declaration
becomes an unqualified identifier within the scope it is in and nested scopes.

- A type declaration is signalled by `++ type`, with the exception of struct
  and enum declarations, which are signalled by `++ struct` and
  `++ enum`, respectively. Though both structs and enums are also types.
- A function is signalled by `++ fun`
- A compile time known variable is signalled by `++ comptime`
- All of the above can be preceded by a `--` instead of `++`, to create a
  private declaration. A private declaration is visible only within the scope
  it was declared in and inner scopes.

```rs
-- type std = @import("std");

++ comptime i32 i = 42;

++ struct Example
{
    i32 integer;
    f32 floating_point;
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
    ?*u8 bytes;
    ?*u64 generations;
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

To create variables stored in the data segment of the program, see
[the `static` keyword section](#static-keyword).

Procedural scope may include expressions which are not assigned. All expressions
and assignments are calls to other procedures (`fun`s) or primitive operations
such as arithmetic, copying, or (de)referencing. These expressions
will have side effects and will be run in consecutive order at runtime when the
`fun` is called. There is a reserved `fun` name, `main`, which is a `fun` which
is the entrypoint of the program.

#### `using` keyword

The members of a struct can be collapsed into another struct where there are no
name collisions between the members of the two structs. For example:

```rs
-- struct Person
{
    u64 age;
    []utf8 name;

    ++ fun printName(*self) {
        std::println("{self.name}");
    }
}

-- struct Member
{
    using Person person;
    u64 join_day;
}

-- struct MemberNonUsing
{
    Person person;
    u64 join_day;
}

++ fun main() {
    Member m = { .age = 42, .name = "Guy", .join_day = 0 };
    MemberNonUsing mnu = {
        .person = { .age = 42, .name = "GuyNonUsing" },
        .join_day = 0,
    };

    // includes member functions and compile time constants, ie. all
    // public declarations + all member variables == all members
    m.printName();

    mnu.person.printName();

    // NOTE: using is different from inheritance because the types cannot
    // implicitly convert

    // *Person inner = m&; // compile error! cannot assign *Member to *Person
    *Person inner = m.person&;
    *Person inner = mnu.person&;
}
```

It is also possible to improve packing by allowing the compiler to mix members
from the inner struct into members from the outer struct. This is achieved by
eliminating the identifier after the struct type:

```rs
-- struct Member
{
    using Person /* person */;
    u64 join_day;
}

++ fun main() {
    Member m = { .age = 42, .name = "Guy", .join_day = 0 };
    m.printName();

    // this is a compile error, same as previous example.
    // *Person p = m&;
}
```

The downside to this is that, because the compiler is free to reorder the
members, you can no longer create a `*Person` from the `Member`, as the fields
of the `Person` substruct may be laid out in a way that functions accepting
`*Person` do not expect.

### `comptime` keyword

Declarations of variables may be marked with `comptime`, enabling them to be
used in comptile time expressions (namely, the default initialization
expressions of struct member variables). For example:

```rs
-- comptime i64 value = 100;

-- struct Example
{
    i64 i = value
}
```

The expression on the right hand side of the equals sign must be able to be
evaluated at compile time. Literals are compile-time known, and arithmetic and
struct initialization can be done at compile time. Referencing other `comptime`
variables is allowed. Calling functions or taking the address of something or
dereferencing a pointer are not allowed.

### Struct member access vs. "namespace" access

Struct members are accessed with the typical `object.member` syntax. This
includes methods and fields. For example:

```rs
-- type std = @import("std");

++ struct Person {
    []utf8 name;
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
        std::print("goodbye");
    }

    ++ u64 size = 64;
};

++ func main() -> i32 {
    my_functions::printHello();
    my_functions::printGoodbye();
    return my_functions::size.to<i32>();
}
```

A file is also a struct. Using the built-in `@import` function you can import
a file and call its functions in the same way you would a struct. For example:

```rs
// file_a.fun -----------------------------------
-- type std = @import("std");

++ fun test() {
    std::println("testing...");
}

// main.fun -------------------------------------
-- type otherfile = @import("file_a.fun");

++ func main() {
    otherfile::test();

    // or, inlined:
    @import("file_a.fun")::test();
}
```

### `static` keyword

The `static` keyword can be used before struct members and before struct
declarations for two similar purposes. When used on a struct
declaration, it means that there is only one instance of the struct, and it is
not possible to instantiate it, and its member methods are called using the
namespace `::` syntax instead of the `.` member access. For example:

```rs
-- static struct print_functions
{
    // NOTE: public member variable declaration. the `++` here only has an
    // effect on static structs, and is harmless (and meaningless) when applied
    // to the members of a nonstatic struct. Basically, nonstatic members are
    // always public and static members are private by default but can be marked
    // public with `++`.
    // NOTE: a default initializer must be provided for pointers so that the
    // struct can implement the `Default` trait. Only Default structs can be
    // `static`. Another option here would be to provide no default and use
    // ?*char instead.
    ++ []utf8 name = "DefaultName";

    ++ fun printHelloName(self) {
        std::println(f"hello, {self.name}");
    }

    ++ fun printGoodbyeName(self) {
        std::println(f"goodbye, {self.name}");
    }
}

++ fun main() {
    print_functions::printHelloName();
    print_functions::printGoodbyeName();
    print_functions::name = "Glooby";
    print_functions::printHelloName();
    print_functions::printGoodbyeName();
}
```

Removing the `static` keyword has no effect on the code within the struct,
however `main()` would need to be rewritten like so:

```rs
++ fun main() {
    print_functions functions = {}; // default things
    functions.printHelloName();
    functions.printGoodbyeName();
    functions.name = "Glooby";
    functions.printHelloName();
    functions.printGoodbyeName();
}
```

To declare a file as `static`, include the static keyword first in the file,
followed by a semicolon:

```rs
// print_functions.fun ----------------------------
static;

++ []utf8 name = "DefaultName";

++ fun printHelloName(*self) {
    std::println(f"hello, {self.name}");
}

++ fun printGoodbyeName(*self) {
    std::println(f"goodbye, {self.name}");
}
```

Finally, `static` can be used before struct members to declare them const. This
keyword means a) the field is initialized by a compile time constant expression
an is not present in designated struct initializers and b) it does not take up
space in actual instances of the struct and c) you cannot modify this field.
This is primarily useful for declaring variables which need to be compile time
constants but should also be able to have their address taken.

### Function arguments and shadowing

Function arguments cannot be `var`. Pointers can point to a `var` type, but the
pointer itself cannot be reassigned.

Variables declared in a procedural scope may be shadowed by other declarations.
For example:

```rs
-- fun myFunction() {
    // both of these values are const
    []utf8 my_string = "Hello";
    []utf8 my_string = "Goodbye"; 
    // type may change
    i64 my_string = 0;
    // constness may change, previous values may be referenced on RHS
    var i64 my_string = my_string;
    // make it const again
    i64 my_string = my_string;
}
```

Shadowing is a purely syntactical feature: references to shadowed variables
remain valid until the end of the scope in which they were declared.

### `self` keyword and member methods

When defining function parameter lists, it is possible to use a pointer to the
special keyword `self` in place of the type and identifier of the first argument.
`*self` may also be `*var self`.

```rs
++ struct Person
{
    []utf8 name;
    u64 age;
    ?*Person child;

    ++ fun setChild(*var self, *Person child) {
        self.child = child;
    }

    ++ fun getChild(*self) -> ?*Person {
        return self.child;
    }
}
```

When calling member methods which take `self` by `*var`, it is necessary to
explicitly use the `var` reference operator:

```rs
++ fun main() {
    Person p = { .name = "Guy", .age = 42 };
    Person p2 = { .name = "Child", .age = 12 };
    if not p.getChild() {
        p&var.setChild(p2&);
    }
}
```

### `if` / `else` and captures

### `while` and captures

### `for` and `Iter` and `RandomAccessIter`

### Tagged blocks

### Option

### Result

### Pointer to Option and Result

### `defer` and `errdefer`

### enums

An enum is a combination of a C-style union and a C-style enum. It contains
both a discriminator (the enum) which identifies which member of the union
is active. The alignment of an enum is equal to the alignment of its
most aligned member, or the alignment of the smallest unsigned integer
type that can uniquely identify all of fields of the enum (unless you have
more than 256 values in the enum, this is usually just a u8). The size of
an enum follows the same rule but with the largest member instead of the
most aligned member.

An enum may have member functions and comptime variable declarations, the
same as a struct.

```rs
-- type std = @import("std");

// a c-style enum
++ enum GraphicsBackend
{
    // unused identifiers followed by a semicolon are interpreted as enum
    // values
    Vulkan;
    DirectX12;
    WebGPU;

    ++ fun print(*self) {
        // this is a worse solution than match statements, but those
        // are in the next section
        if self.* == Self::Vulkan {
            std::println("Vulkan");
        } else if self.* == Self::DirectX {
            std::println("DirectX 12");
        } else if self.* == Self::WebGPU {
            std::println("WebGPU");
        }
    }
}

// usage of c style enum
++ static struct GlobalGraphicsContext
{
    ?GraphicsBackend chosen_backend;

    ++ fun init(*var self, GraphicsBackend backend) {
        self.chosen_backend = backend;
        backend.print();
    }

    ++ fun initVulkan(*var self) {
        self.chosen_backend = GraphicsBackend::Vulkan;
        GraphicsBackend::Vulkan.print();
    }
}
```

But enums are also tagged unions, so enum variants can contain a type:

```rs
-- type vulkan = @import("vulkan");
-- type directx = @import("vulkan");

++ enum GraphicsContext {
    VulkanImpl vulkan::ctx;
    DirectXImpl directx::ctx;
    // TODO: is this anonymous struct thing cool and simple or overcomplicated. I cannot tell
    SoftwareImpl {
        // another struct scope!
        *[500]u8 pixbuf;
        // ...
    };
};
```

#### Pattern matching

Pattern matching is used to access the inner elements of an enum or optional
which may be nested. For example, you may have an optional enum, whose variant
may contain an optional `i64`. You could make nested if statements to check for
the outer optional not being null, the enum being the optional `i64`-containing
variant, and the `i64` not being null, but that would be clunky and cause
unecessary nesting. To solve this, there are custom control flow primitives to
enable paths with access to certain inner variables of types are only accessible
if they also exist within the object.
Thse control flow primitives are:

- `if is`: Similar to `if let` in rust, this checks if a variable can be
  matched by a given pattern and captures the result, and only runs the
  resulting block if the match was successful.
- `is ... else`: Similar to `let else` in rust, perform a pattern match
  and assign the resulting inner value to a left-hand-side variable, otherwise
  run the contents of the `else` block. The match statement always must include
  a capture, and the capture is elided in favor of assigning to the LHS identifier.
- `while is`: Identical to `if is` but for the conditional of a while
  loop rather than an `if` block.
- `match`: perform many possible pattern matches, executing only one case
  (branch) where the match was successful.

The syntax for pattern matching is as follows:

```txt
<<typename | ?> [typename | ?...]<literal | (<capture | _>)> | (<capture | _>) | <literal>>
```

For example, given the following types:

```rs
-- enum FooBar
{
    Foo ?i64;
    Bar;
    Baz;
    Zig;
    Zag;
}

++ fun someFunction() -> ?FooBar { /* ... */ }
```

To perform an access to the `i64` inside the `Foo` variant returned from
`someFunction` using `is else`, eliminating the capture `(<capture | _>)` and
instead performing assignment:

```rs
++ fun main() -> i32 {
    i64 inner_int = someFunction() is ?FooBar::Foo else {
        // someFunction didn't return a FooBar::Foo, so we can't use the int
        // inside it
        return 1;
    };

    // do something with inner_int
}
```

To perform the same using an `if is`:

```rs
++ fun main() -> i32 {
    if someFunction() is ?FooBar::Foo(inner_int) {
        // do something with inner_int
    } else {
        // someFunction didn't return a FooBar::Foo, so we can't use the int
        // inside it
        return 1;
    }

    // also valid: no capture
    if someFunction() is ?FooBar::Foo {
        // ...
    }
}
```

To create something like an iterator in userspace, use `while is` and a
generator/iterator object:

```rs
++ struct OptionalFooBarGenerator { /* */ }

++ fun createOptionalFooBarGenerator() -> OptionalFooBarGenerator { /* ... */ }

++ fun next(*var OptionalFooBarGenerator) -> ??FooBar { /* ... */ }

++ fun main() -> i32 {
    var OptionalFooBarGenerator gen = createOptionalFooBarGenerator();

    // loop until one of the items is not a FooBar::Foo
    while next(&var gen) is ??FooBar::Foo(inner_int) {
        // do something with inner_int
    }
}
```

Then there are `match` statements. Match statements are most obviously similar
to switch statements in C, and are almost functionally identical when used on a
c-style enum (an enum where none of the variants contain data). For example:

```rs
++ type std = @import("std");

-- enum FooBar
{
    Foo;
    Bar;
    Baz;
    Zig;
    Zag;
}

++ fun someFunction() -> FooBar { /* ... */ }

++ fun main() {
    match someFunction() {
        FooBar::Bar => std::println("bar");
        FooBar::Baz => std::println("baz");

        // multiple lines must be encased in a block
        FooBar::Foo => {
            std::println("this will print, I hope!");
            std::println("foo");
        }

        FooBar::Zig => std::println("zig");
        FooBar::Zag => std::println("zag");
    }
}
```

The above example only matches on literals, but a match statement may include
captures instead of literals.

```rs
FooBar myfoo = FooBar::Foo;

match myfoo {
    FooBar::Bar => std::println("bar");
    FooBar::Baz => std::println("baz");
    (anything_else) => std::println(f"got {anything_else.to<int>()}");
}
```

Using a capture for a structural part of a match is effectively saying
"take anything else (something that isn't one of the literals I have
tried to match on) and put it in this variable." For this reason, a
capture match clause must always be written after literal match clauses
of the same part of the match. The capture is effectively the "else."
For this reason, `_` can be thought of as the "default" case. It is
a structural match on anything not already matched at any level of
the structure of the type.

```rs
match myfoo {
FooBar::Bar => std::println("bar");
(_) => std::println("anything else");
}
```

Unlike a switch statement in C, there is no fallthrough. If you want to
achieve fallthrough, you can use an if/else if/else chain :).

Match statements can match on options. Write the `?` type decorator
in the same position as it appears on the unmatched type in order to
match on active optionals. And use `null` to match on an unused/empty/inactive
optional.

```rs
++ fun someFunction() -> ?i64 {
    // ...
}

++ fun main() {
    match someFunction() {
        ?(return_value) => {
            std::println(f"got {return_value}");
        };
        null => @panic("failure");
    };
}
```

Here is a more complex example, also including `is else` syntax

```rs
-- type std = @import("std");

-- enum FooBar
{
    Foo ?i64;
    Bar;
    Baz;
    Zig;
    Zag;
}

++ fun someFunction() -> ?FooBar { /* */ }

++ fun main() {
    i64 matched = someFunction() is ?FooBar::Foo ? else {
        return 1;
    };

    match someFunction() {
        ?FooBar::Bar => std::println("bar");
        ?FooBar::Baz => std::println("baz");
        ?FooBar::Zig => std::println("zig");
        ?FooBar::Foo null => @panic("got nothing for inner integer");
        ?FooBar::Foo 12 => @panic("oh no... twelve");
        ?FooBar::Foo (inner_int) => std::println(f"got {inner_int}");
        null => @panic("got nothing");
        (_) => std::println("unrecognized return value");
    }
}
```

### Slices and arrays

### String literals

### `f`-strings

### Vector and matrix types

### context
