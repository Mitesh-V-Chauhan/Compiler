.text
.globl _main
_main:
    pushq %rbp
    movq %rsp, %rbp
    subq $256, %rsp
entry0:
    leaq .L.str0(%rip), %rax
    movq %rax, %rdi
    movq $42, %rax
    movq %rax, %rsi
    xor %eax, %eax
    call _printf
    movq %rax, -8(%rbp)
    movq $0, %rax
    movq %rbp, %rsp
    popq %rbp
    ret
.section __TEXT,__cstring,cstring_literals
.L.str0:
    .asciz "Hello from Nova! The number is %d\n"
