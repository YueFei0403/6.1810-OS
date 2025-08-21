## Command History Navigation in xv6 Shell

The shell maintains a circular buffer of past commands:

- `history_count` = total number of saved cmds
- `history_index` = current browsing position
    -  `0 ... history_count-1` → valid history entries
    - `history_count` → special "blank line" state (fresh prompt, no history)

When handling **Down arrow** (`↓`):
1. We first check `if (history_index < history_count)` before incrementing.
    - This prevents moving beyond the blank state.
2. After incrementing, we check **again**:
    - If `history_index < history_count`: copy `history[history_index]` into the input buffer.
    - Else (`history_index == history_count`): clear the buffer and display a blank prompt.

Why not `<=` in the **second check**?
- Because `history[history_count]` does not exist - it would be an *out-of-bounds* array access. 
- The blank state is represented by the *absence* of an entry, so it must be handled separately.

This two-stage check ensures:
- Safe bounds checking
- Correct behavior when scrolling past the newest cmd
- Consistency with modern shells (Bash, Zsh), which returns to a blank line after reaching the end.

---

### History Index Navigation Diagram

#### Example history:
```c
history[0] = "ls"
history[1] = "cat"
history[2] = "echo"
history_count = 3
```

#### State Machine:
```
[0] "ls" ←─── [1] "cat" ←─── [2] "echo" ←─── [3] "" (blank line)
```

- `0...history_count - 1` → valid commmands
- `history_count` → special "blank line" state (empty buffer, ready for new input)


#### Key Behavior:
- Pressing ↑ (up) decrements `history_index` → shows older cmds
- Pressing ↓ (down) increments `history_index` → shows newer cmds
- When `history_index == history_count`, the buffer is cleared → users sees a blank prompt (just like Bash/Zsh)