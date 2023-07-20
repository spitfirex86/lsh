## how to use ???
Behold, the world's worst documentation for the world's worst shell/scripting language that operates entirely on DLLs and imported functions: **lsh**

### Part 1, some basic syntax
- import a library: `$Lib("user32.dll")`, `$Lib(msvcrt)` \
extension optional (assumes .dll if not provided), quotes optional unless the name contains whitespace. *more on that later*

- call a library function: `MessageBoxA(0, "Message", "Title", 0x40)`, `malloc(32)`

- assign a variable: `?foo = malloc(260)` \
variables can store a number or a pointer. no builtin string type of any kind (unless when passed as a parameter), deal with it. *more on that later* \
to assign a number to a variable, use`$Val()` : `?bar = $Val(1234)` 

- comments: `; this is a comment`. comments must start on a new line! sorry for the inconvenience, if it wasn't clear until now this whole thing is very janky

- last return value: `$Ret` <-- this is an important feature! \
all library function calls, script function calls, assignments and operators will affect this value. *some* things will implicitly use it when provided with no parameters. *more on this later!*

- operators - all the basic relational ones should be supported i think. call them like a function: `$==(?bar, 1337)`, `?notNull = $!=(?var, 0)`

- conditionals - you get a very basic if, that's it:
```
{$If(?var):
    ; something happens here
}
```
right now you cannot use the relational operators directly as a parameter (oops), so using $If without any parameters will implicitly take the last return value (`$Ret`):
```
; compare some variable. (this implicitly sets $Ret)
$>(?var, 4)
{$If:
    ; something happens here. (equivalent to `$If($Ret)`)
}
```
- define a script function:
```
{MsgBox:
    $Lib(msvcrt)
    MessageBoxA(0, ?1, ?2, 1)
}
```
the last value is implicitly returned - in this example, it's the result of MessageBoxA. \
`?1`, `?2`, ... are automatically assigned parameters

- call a script function: `:MsgBox("text", "title")`


### Part 2, The Quirks
- right now, there's no builtin way to do math. you cannot add or increment a number. sorry not sorry
- parens are optional when no parameters are passed: `Foo()` == `Foo`
- all variables are passed by value. use them as pointers if you must. if you need to dereference, you're SOL
- all variables are global, except for the auto parameter vars
- **All you need to know about `$Ret`:**
  - every action that has a value will set this builtin var. you can also assign it at will by just placing a variable on a line with no other actions. use this to return a specific value from a function
  - you can use it as you would any regular variable. for example, the following two functions are equivalent:
```
{Malloc1:
    $Lib(msvcrt)
    ?tmp = malloc(?1)
    memset(?tmp, 0, ?1)
    ?tmp
      ; this explicitly sets the return value to ?tmp
}
{Malloc2:
    $Lib(msvcrt)
    malloc(?1)
    memset($Ret, 0, ?1)
      ; memset returns its first param so this is also fine
}
```
- scripts read from file are equivalent to just typing the whole thing line by line in the shell
- you can only have 20 named variables
- need to store a string in a variable? here's one way to do this:
```
{Str:
    $Lib(msvcrt)
    ; cool trick to get length + 1
    _snprintf(0,0, "%s ", ?1)
    malloc($Ret)
    strcpy($Ret, ?1)
}

?foo = :Str("hello world")
; maybe free it at some point?
```


### Part 2.5, String-Related Quirks:tm:
- all parameters are technically strings until the moment they are evaluated
- strings don't actually have to be quoted... `Foo("bar")` == `Foo(bar)`, **except:**
    - `Foo("123")` != `Foo(123)`, left is a string, right is converted to a number (if applicable)
    - `Foo(lorem ipsum)` == `Foo("loremipsum")`, spaces in unquoted strings are stripped (don't ask about this one)
    - `Foo(?bar)` != `Foo("?bar")`, left is a variable reference, right is a string
- `Foo()` == no params, but `Foo(,)` == `Foo(" ", " ")`

there's probably more i forgot about...


### Part 3, just deal with it
don't forget the main feature of lsh is **calling functions in libraries**. it was not, and still is not intended to be a full scripting language.

if you need math: feel free to create and import your own DLL with Add(), Subtract(), etc...

need to dereference a pointer? that's right, make your own library with a Dereference() function.

have fun.


(i may or may not introduce more features in the future, but i don't really care right now)
