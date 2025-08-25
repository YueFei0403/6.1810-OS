## `ttyraw` Syscall Flow in xv6 (RISC-V)

This document explains the end-to-end call chain for the custom `ttyraw(int on)` system call that enables or disables raw input mode in xv6.

---

### 1. User Space (Shell)
In `user/sh.c`:
```c
int main(void) {
    ttyraw(1);  // enable raw mode for this process
    // ... shell loop ...
}
```

Declaration:
In `user/user.h`:
```c
int ttyraw(int on);
```

Syscall Stub:
In `user/usys.pl`:
```c
entry('ttyraw');
```
This generates assembly in `user/usys.S`
```c
ttyraw:
    li a7, SYS_ttyraw
    ecall
    ret
```

---

### 2. Trap into Kernel
When the program executes `ecall`:
1. The RISC-V CPU switches from *user mode to supervisor mode*.
2. The hardware saves the user PC into `sepc` and jumps to the *trap vector* (set up in `kernel/trampoline.S`).
3. The kernel trap handler (`kernel/trap.c`) runs, which:
    - Saves all user registers into the process's **trapframe** (part of `struct proc`).
    - Switches to the kernel page table and stack.
    - Calls `syscall()` if the trap cause was an environment call (ECALL) from user mode.

#### Syscall Entry Point
In `kernel/syscall.c`:
```c
void
syscall(void)
{
    int num;
    struct proc *p = myproc();

    num = p->trapframe->a7;
    if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
        p->trapframe->a0 = syscalls[num](); // call handler
    } else {
        p->trapframe->a0 = -1;
    }
}
```
Notes:
- `a7` contains the syscall number (`SYS_ttyraw`).
- The dispatcher uses `num` as an index into the `syscalls[]` array.
- `syscalls[num]` is a function pointer (the address of the handler).
- `syscalls[num]()` actually invokes the handler and returns its result.

Registers:
- `a0` = return value of the syscall
- `a7` = syscall number (`SYS_ttyraw`)

---

### 3. Syscall Dispatch Table
In `kernel/syscall.h`:
```c
#define SYS_ttyraw 22    // next free number
```

In `kernel/syscall.c`:
```c
extern uint64 sys_ttyraw(void);

static uint64 (*syscalls[])(void) = {
    // ...
    [SYS_ttyraw] sys_ttyraw,
};
```
**How this works**
- `syscalls[]` is an array of function pointers
- Each index corresponds to a syscall number defined in `syscall.h` (e.g., `#define SYS_read 5`).
- Example: `syscalls[SYS_read] = sys_read;`
    - `sys_read` is the kernel function that handles the `read()` syscall
- When a syscall is invoked:
    - The number (from register `a7`) is used as an index.
    - The kernel looks up the pointer in `syscalls[num]`.
    - It calls it with `syscalls[num]()`.

So:
- `syscalls[num]` = the pointer to the handler function.
- `syscalls[num]()` = call the handler.


---

### 4. Syscall Implementation
In `kernel/sysfile.c` (or similar):
```c
uint64
sys_ttyraw(void)
{
    int on;
    argint(0, &on);     // fetch first syscall argument
    console_set_rawmode(on);    // update console state
    return 0;
}
```
**Argument Fetching**
`argint(n, &on)` copies the nth syscall argument (from trapframe register `a0`, `a1`, ...) into a kernel variable.

### 5. Console Driver Hook
In `kernel/console.c`:
```c
void
console_set_rawmode(int on)
{
    acquire(&cons.lock);
    cons.rawmode = on;      // 0 = cooked, 1 = raw
    release(&cons.lock);
}
```
`cons.rawmode` determines how `consoleinstr()` handles input:


- Cooked mode (0): buffer until newline, kernel echoes input.
- Raw mode (1): deliver each key immediately, no kernel echo.

### 6. Back to User Space
After `sys_ttyraw` (`kernel/sysfile.c`) returns, control resumes in the user program. The shell can now handle raw input (arrow keys, backspace, tab completion, etc.) on its own.

✅ Summary
- User calls `ttyraw(1)` in `sh.c`.
- Stub in `usys.S` sets `a7 = SYS_ttyraw` and executes `ecall`.
- CPU traps into kernel, saves state, and runs `syscall()` dispatcher.
- `syscalls[num]` is the array of function pointers mapping syscall numbers of handlers.
- `syscalls[num]()` calls the correct handler (e.g., `sys_ttyraw`).
- `sys_ttyraw` fetches argument with `argint` and calls `console_set_rawmode`.
- Console driver updates `cons.rawmode`.
- Trap returns to user space with new input mode active.


--- 
`syscalls[num]`
- This is just the function pointer stored in the array.
- Its type (in `xv6`) is `uint64 (*)(void)` -- meaning "pointer to a function taking no args, returning a `uint64`".
- If you print it (e.g. `%p`), you'll see the address of the function.
- But it does not *execute* the function.

**Example**:
```c
printf("%p\n", syscalls[SYS_write]); // prints address of sys_write
```

`syscalls[num]()`
- The parentheses mean: call the function the pointer is pointing to.
- So if `syscalls[num] = sys_ttyraw`, then
```c
syscalls[num]()
```
actually runs `sys_ttyraw()`, and returns its `uint64`.

#### Syscall Dispatcher Diagram
```
+-----------+-------------------+
| Index     | Handler Function |
+-----------+-------------------+
| 0         | unused |
| 1         | sys_fork |
| 2         | sys_exit |
| 3         | sys_wait |
| ...       | ... |
| 5         | sys_read |
| 16        | sys_write |
| 22        | sys_ttyraw |
+-----------+-------------------+

        a7 = 22
            │
            ▼
        syscalls[22]
            │
            ▼
        sys_ttyraw()
            │
            ▼
        argint(0, &on) → fetch syscall argument
            │
            ▼
        console_setraw(on)
            │
            ▼
        cons.rawmode = on (0 = cooked, 1 = raw)
```
