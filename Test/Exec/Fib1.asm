fib:
    stp  sp, 16
    str  x0, [sp, 0]
    cmp  x0, 2
    blt  retn
    sub  x2, x0, 2
    str  x2, [sp, 8]
    sub  x0, x0, 1
    bl   fib
    ldr  x2, [sp, 8]
    str  x3, [sp, 8]
    mov  x0, x2
    bl   fib
    ldr  x1, [sp, 8]
    add  x3, x1, x3
    ldp  sp, 16
    ret
retn:
    ldr  x3, [sp, 0]
    ldp  sp, 16
    ret

main:
    mov  x9, 0 
top:
    cmp  x9, 25
    beq  done
    mov  x0, x9
    bl   fib
    prg  x3
    inc  x9
    b    top
done:
    mov  x0, 0
    ret
