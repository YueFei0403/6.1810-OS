## `xv6` Console Note - Part 1

### The ring buffer
```c
#define INPUT_BUF_SIZE 128
char buf[INPUT_BUF_SIZE]
uint r; // Read index
uint w; // Write index
uint e; // Edit index
```
Think of `buf[]` as a circular queue of characters typed by the user.
- The shell (user space) reads characters out with `read(0,...)`.
- The kernel (console driver) fills it in as keystrokes arrive. 

---

`r` -- Read Index
- Who updates it? -> `consoleread()`.
- Points to the *character to give to user space*.
- When the shell does `read(0, &c, 1)`, it consumes `buf[r]` and then increments `r`.


In `xv6`: 
- The shell (or any program) calls:
```c
read(0, &c, 1);
```
This asks the kernel: *"please give me the next byte of input from stdin."*

- The kernel's `sys_read()` eventually calls `consoleread()` if `fd==0` (stdin).
- `consoleread()` uses the console buffer (`cons.buf[]`) and the indices (`r`, `w`, `e`) to decide:
    - Is there data available (`r < w`)?
    - If yes, copy `cons.buf[r % INPUT_BUF_SIZE]` into user space (at address `&c` above).
    - Increment `r`.

So in practice
- `r` = index of the next byte that will be copied into the program's buffer when it calls `read()`.
- "Give to user space" = copy from `cons.buf[r]` (*kernel memory*) -> into the process's memory (*user space*).

**Example: You type `a b <Enter>`**

**Step 1. Typing characters**
- You press `'a'`:
    - Stored in `cons.buf[0]`
    - `e = 1`, `w` still old, `r` unchanged
- You press `'b'`:
    - Stored in `cons.buf[1]`
    - `e = 2`
- You press `<Enter>` (`'\n'`)
    - Stored in `cons.buf[2]`
    - `e = 3`
    - Now kernel commits: `w = e = 3`

At this moment: 
```c
buf: ['a']['b']['c']
r = 0   // nothing read yet
w = 3   // 3 chars committed
e = 3   // also 3, since editing finished
```

**Step 2. User program (the shell) reads**
- First `read(0, &c, 1)` --> gives `'a'`
    - Kernel copies `buf[0]` -> user buffer
    - then `r = 1`
- Second `read()` --> gives `'b'`
    - Kernel copies `buf[1]`
    - then `r = 2`
- Third `read()` -> gives `'\n'`
    - Kernel copies `buf[2]`
    - then `r = 3`

**Step 3: End of Line**
Now:
```c
r = 3
w = 3
e = 3
```
So in this example the final value of `r` is 3 (equal to `w` and `e`).

That means the user program has consumed everything the user typed


