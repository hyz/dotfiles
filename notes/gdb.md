
    (gdb) set variable input.last_player = 0

### https://stackoverflow.com/questions/3305164/how-to-modify-memory-contents-using-gdb

    (gdb) set {int}0x83040 = 4

