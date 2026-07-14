.text
.globl _main
_main:
    pushq %rbp
    movq %rsp, %rbp
    subq $256, %rsp
entry0:
    leaq -8(%rbp), %rax
    addq $0, %rax
    movq %rax, -16(%rbp)
    movq $20, %rcx
    movq -16(%rbp), %rax
    movq %rcx, (%rax)
    leaq -8(%rbp), %rax
    addq $8, %rax
    movq %rax, -24(%rbp)
    movq $30, %rcx
    movq -24(%rbp), %rax
    movq %rcx, (%rax)
    leaq -8(%rbp), %rax
    addq $0, %rax
    movq %rax, -32(%rbp)
    movq -32(%rbp), %rax
    movq (%rax), %rcx
    movq %rcx, -40(%rbp)
    leaq -8(%rbp), %rax
    addq $8, %rax
    movq %rax, -48(%rbp)
    movq -48(%rbp), %rax
    movq (%rax), %rcx
    movq %rcx, -56(%rbp)
    movq -40(%rbp), %rax
    movq -56(%rbp), %rcx
    addq %rcx, %rax
    movq %rax, -64(%rbp)
    movq -64(%rbp), %rax
    movq %rax, -72(%rbp)
    movq -72(%rbp), %rax
    movq %rbp, %rsp
    popq %rbp
    ret
