## Process Memory Layout

High Addresses
```
+---------------------------+
|         Stack             |
| - Function calls          |
| - Local variables         |
| - Return addresses        |
| (Grows downward ↓)        |
+---------------------------+
        
        Guard Page
   (unmapped, PROT_NONE)

+---------------------------+
|          Heap             |
| - malloc/free memory      |
| - Dynamic data structures |
| (Grows upward ↑)          |
+---------------------------+

+---------------------------+
|   BSS Segment             |
| - Uninitialized globals   |
| - static int x;           |
| - Zero-initialized vars   |
+---------------------------+

+---------------------------+
|   Data Segment            |
| - Initialized globals     |
| - static int x = 5;       |
+---------------------------+

+---------------------------+
|   Text Segment (Code)     |
| - Compiled instructions   |
| - Read-only constants     |
+---------------------------+
Low Addresses
```

---

### Stack vs Heap in terms of allocation discipline
- **Stack = LIFO (Last In, First Out)**
    - Functions push frames when called, pop them when returning.
    - Allocation/freeing order is strictly nested

- **Heap = No strict order**
    - You can `malloc` memory at any time, and `free` it in any order.
    - There's no requirement that the most recently allocated block must be freed first (unlike the stack).
    - The heap manager (`malloc` implementation) keeps track of free/used blocks with data structures (often free lists, arenas, or trees).


---

### How They Interact
- **Stack**: Each function call pushes a new stack frame (arguments, locals). Freed automatically on return.
- **Heap**: Flexible memory pool used by `malloc/new`. Freed only when you explicitly `free/delete`.
- **Data/BSS**: Store global/static variables. Persist for the entire program.
- **Text**: Machine code for your program (read-only, can't modify instructions at runtime).

---

### Example Code Walkthrough
```c
int g1 = 5; // initialized      --> Data
int g2;     // uninitialized    --> BSS

void foo() {
    int local = 10;         // Stack
    int *p = malloc(100);   // Heap
    *p = 42;
    free(p);
}

int main() {
    foo();
}
```