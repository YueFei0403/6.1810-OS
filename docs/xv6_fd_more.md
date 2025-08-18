# Example: `ls | grep foo`

## Step 1 — Shell creates a pipe
```c
int p[2];
pipe(p);   // p[0] = read end, p[1] = write end
```

```
Shell FD Table
 ------------------------
 fd=0 → stdin (console)
 fd=1 → stdout (console)
 fd=2 → stderr (console)
 fd=3 → pipe read end   (p[0])
 fd=4 → pipe write end  (p[1])
```

---

## Step 2 - Fork first child (`ls`)
```c
dup2(p[1], 1);  // stdout -> pipe write
close(p[0]);    // we don't read
close(p[1]);    // now we write to to stdout -> so close pipe[1]
```

```
Child1 (ls) FD Table
 -----------------------
 fd=0 -> stdin (console)
 fd=1 -> pipe write end (dup of fd=4)
 fd=2 -> stderr (console)
```

---

## Step 3 - Fork second child (`grep foo`)
```c
dup2(p[0], 0);  // stdin -> pipe read
close(p[0]);    // now we read from stdin -> close pipe[0]
close(p[1]);    // we don't write
```

```
Child2 (grep) FD Table
 ------------------------
 fd=0 → pipe read end (dup of fd=3)
 fd=1 → stdout (console)
 fd=2 → stderr (console)
```

---

## Step 4 - Shell cleans up
*Parent closes both ends of the pipe and waits*:
```
Shell FD Table
 ------------------------
 fd=0 → stdin (console)
 fd=1 → stdout (console)
 fd=2 → stderr (console)
```

---

## ✅ Data Flow
```
   [ls stdout]
        |
        v
   pipe write end (fd=1 in ls)
        |
   ───── buffer in kernel ─────
        |
   pipe read end (fd=0 in grep)
        v
   [grep stdin]
```


