section .data
    data:  dd 45, 12, 78, 34, 56, 255, 100, 23   ; 配列データ
    ndata equ 8                  ; 配列の要素数

section .text
    global _start

_start:
    mov ecx, data         ; 配列の要素数をecxに格納
    mov esi, ndata          ; dataの値を入れる
    
Outloop:
    dec esi
    jz end            ;ecx = 0 で終了
    mov edi, ecx
    mov edx, esi

Inloop:
    mov eax, [edi]         ;今の値
    mov ebx, [edi + 4]     ;次

    cmp eax, ebx
    jbe Noswap       ; eax >= ebx swapしない

    mov [edi], ebx
    mov [edi + 4], eax

Noswap:
    add edi, 4
    dec edx
    jnz Inloop
    jmp Outloop

end:
    mov eax, 1
    mov ebx, 0
    int 0x80