
[org 0x7C00]        ; Bootloader origin address
[bits 16]           ; 16-bit real mode

start:
    ; Set up segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00  ; Stack pointer just below bootloader

    ; Set video mode to 80x25 text
    mov ax, 0x0003
    int 0x10

    ; Hide cursor
    mov ah, 0x01
    mov cx, 0x2607
    int 0x10

    ; Clear screen
    mov ax, 0x0700  ; Scroll entire window
    mov bh, 0x07    ; White on black
    xor cx, cx      ; Upper left corner (0,0)
    mov dx, 0x184f  ; Lower right corner (24,79)
    int 0x10

    ; Get random message variant (1-5) using timer tick
    mov ah, 0x00    ; Get system timer
    int 0x1A        ; CX:DX = timer ticks
    mov ax, dx
    xor dx, dx
    mov cx, 5
    div cx          ; DX = remainder (0-4)
    inc dx          ; DX = 1-5
    mov [variant], dx

    ; Set message pointer based on variant
    mov si, message1  ; Default to variant 1
    cmp dx, 2
    jb .set_message
    mov si, message2
    cmp dx, 3
    jb .set_message
    mov si, message3
    cmp dx, 4
    jb .set_message
    mov si, message4
    cmp dx, 5
    jb .set_message
    mov si, message5
.set_message:
    mov [msg_ptr], si

    ; --- PHASE 1: Slow printing of initial message ---
    mov si, [msg_ptr]
    call strlen      ; Get message length in CX
    mov di, 0       ; Starting position (top-left)

phase1_loop:
    ; Calculate cursor position (row in dh, column in dl)
    mov ax, di
    mov bl, 80
    div bl          ; al = row, ah = column
    mov dh, al      ; Row
    mov dl, ah      ; Column

    ; Set cursor position
    mov ah, 0x02
    mov bh, 0       ; Page 0
    int 0x10

    ; Print character
    mov ah, 0x0A
    mov al, [si]    ; Character to print
    mov bh, 0       ; Page 0
    push cx
    mov cx, 1       ; Print 1 character
    int 0x10
    pop cx

    ; Next character
    inc si
    inc di

    ; Check for message end
    cmp byte [si], 0
    jne .no_wrap1
    mov si, [msg_ptr] ; Wrap message
.no_wrap1:

    ; Delay
    push cx
    mov cx, 0x0002  ; Delay amount (higher = slower)
    mov dx, 0x0000
    mov ah, 0x86
    int 0x15
    pop cx

    loop phase1_loop

    ; --- PHASE 2: Rapidly fill entire screen ---
    xor di, di      ; Reset position counter (0-1999)
    mov si, [msg_ptr]

phase2_loop:
    ; Calculate cursor position (row in dh, column in dl)
    mov ax, di
    mov bl, 80
    div bl          ; al = row, ah = column
    mov dh, al      ; Row
    mov dl, ah      ; Column

    ; Set cursor position
    mov ah, 0x02
    mov bh, 0       ; Page 0
    int 0x10

    ; Print character
    mov ah, 0x0A
    mov al, [si]
    mov bh, 0
    mov cx, 1
    int 0x10

    ; Next character in message
    inc si
    cmp byte [si], 0
    jne .no_wrap2
    mov si, [msg_ptr] ; Wrap message
.no_wrap2:

    ; Next screen position
    inc di
    cmp di, 80*25   ; Check if we filled the screen
    jge .done

    jmp phase2_loop

.done:
    jmp $           ; Infinite loop when done

; Calculate string length (input: SI, output: CX)
strlen:
    push si
    xor cx, cx
.count_loop:
    lodsb
    cmp al, 0
    je .count_done
    inc cx
    jmp .count_loop
.count_done:
    pop si
    ret

; Data
variant db 1
msg_ptr dw message1

message1 db 'THERES NO WAY OUT YOU ARE TRAPPED ',0
message2 db 'Theres nothing you can do. All you do is just staring at this blank screen with text filling.',0
message3 db 'All hope is lost.',0
message4 db 'Theres no way fixing this.',0
message5 db 'WHY MUST GOD FOSAKEN ME??!?!?!',0


; Boot signature
times 510-($-$$) db 0
dw 0xAA55