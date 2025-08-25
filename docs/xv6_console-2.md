## Console Input/Output in `xv6` vs Linux TTY

This doc explains how `xv6` handles console input/output, focusing on `consputc`, backspace, raw vs cooked modes, and how it compares with Linux's TTY line discipline. It also traces down to the lowest level: `artputc_sync`.


### 1. `xv6` Console Output: `consputc`
In `xv6`, `consputc()` is the low-level routine that prints a character to the UART (QEMU serial console). It has special handling for **backspace**.
```c
void consputc(int c) {
    if (c == BACKSPACE) {
        uartputc_sync('\b');    // move cursor left
        uartputc_sync(' ');     // overwrite with space
        uartputc_sync('\b');    // move cursor left again
    } else {
        uartputc_sync(c);
    }
}
```

- **Normal characters** are written directly with `uartputc_sync(c)`.
- **BACKSPACE** (defined as `0x100` in `console.c`) is not a real ASCII code. Instead, xv6 treats it specially and emits `"\b \b"` to visually erase the last character on screen.

### 2. Buffer Pointers in `xv6`
The console keeps a circular input buffer:
- `cons.r` -> read pointer (next char `read()` will consume).
- `cons.w` -> write/publish pointer (chars made available to user space).
- `cons.e` -> edit pointer (where user is currently typing).

#### Timeline Example (INPUT_BUF_SIZE = 8)
##### Typing "hello" (no newline yet):
```c
cons.r = 0      cons.w = 0      cons.e = 5
buf: [h, e, l, l, o, _, _, _]
```
##### Press newline (`\n`):
```c
cons.r = 0  cons.w = 6      cons. e = 6
=> "hello\n" delivered to user space
```
##### Buffer fills (8 chars typed, no newline):
```c
cons.r = 0
cons.w = 0
cons.e = 8 (== cons.r + INPUT_BUF_SIZE)
```
##### At this point, `xv6` flushes automatically:
```c
if (cons.e - cons.r == INPUT_BUF_SIZE) {
    cons.w = cons.e;
    wakeup(&cons.r);
}
```

### 3. Raw Mode vs Cooked Mode in `xv6`
- Cooked mode (default):
    - Echo characters with consputc
    - Buffer input until newline, Ctrl-D, or buffer full.
    - Handle editing in kernel (`backspace`, `Ctrl-U`).

- Raw mode (patch):
    - Deliver characters immediately to user space
    - Kernel does no editing
    - User program (`readline`) must handle backspace, history, tab completion. 


### 4. Comparison with Linux TTY
Linux has a full TTY subsystem with line discipline (`N_TTY`).

**Canonical mode (`ICANON` on):**
- Input buffered until newline.
- Special chars interpreted:
    - `ERASE` (Backspace/DEL) -> erase one char.
    - `WERASE` (Ctrl-W) -> erase word.
    - `KILL` (Ctrl-U) -> erase line.
    - `EOF` (Ctrl-D) -> end input.
- Kernel handles editing (like xv6 cooked mode).

**Non-canonical mode (`stty raw -echo`):**
- Character delivered immediately.
- No echo, no line editing in kernel.
- User program (bash, vim, ssh, etc.) must implement:
    - Backspace (`127` or `8`).
    - Arrow keys (`ESC [ A`).
    - History navigation.
    - Tab completion.

### 5. Input Path: `xv6` vs `Linux`
**xv6 (cooked mode):**
```c
Keyboard -> consoleintr() -> buffer (cons.e) -> consputc echo/backspace -> (on newline) cons.w advanced -> read() -> shell
```
**xv6 (raw mode):**
```c
Keyboard → consoleintr() → buffer (cons.e, cons.w advanced immediately)
         → read() → shell → shell handles editing
```
