.text
.globl _add
_add:
    pushq %rbp
    movq %rsp, %rbp
    subq $256, %rsp
entry0:
    movq %rdi, -8(%rbp)
    movq %rsi, -16(%rbp)
    movq -8(%rbp), %rax
    movq (%rax), %rcx
    movq %rcx, -24(%rbp)
    movq -16(%rbp), %rax
    movq (%rax), %rcx
    movq %rcx, -32(%rbp)
    movq -24(%rbp), %rax
    movq -32(%rbp), %rcx
    addq %rcx, %rax
    movq %rax, -40(%rbp)
    movq -40(%rbp), %rax
    movq %rbp, %rsp
    popq %rbp
    ret
.globl _main
_main:
    pushq %rbp
    movq %rsp, %rbp
    subq $256, %rsp
entry1:
    movq $20, %rax
    movq %rax, -8(%rbp)
    movq $30, %rax
    movq %rax, -16(%rbp)
    leaq -8(%rbp), %rax
    movq %rax, -24(%rbp)
    movq -24(%rbp), %rax
    movq %rax, -32(%rbp)
    leaq -16(%rbp), %rax
    movq %rax, -40(%rbp)
    movq -40(%rbp), %rax
    movq %rax, -48(%rbp)
    movq $50, %rcx
    movq -32(%rbp), %rax
    movq %rcx, (%rax)
    movq -32(%rbp), %rax
    movq %rax, %rdi
    movq -48(%rbp), %rax
    movq %rax, %rsi
    call _add
    movq %rax, -56(%rbp)
    movq -56(%rbp), %rax
    movq %rbp, %rsp
    popq %rbp
    ret
