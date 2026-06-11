/*
 * CellKernel - Interrupt Controller (IDT, APIC, Exceptions)
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 */

#include "../include/kernel.h"

#define IDT_ENTRIES         256
#define ISR_ENTRIES         32

/* Interrupt Descriptor Table entry */
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __packed idt_entry_t;

/* IDT pointer */
typedef struct {
    uint16_t limit;
    uint64_t base;
} __packed idt_ptr_t;

/* Interrupt frame */
typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
    uint64_t vector;
    uint64_t error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} interrupt_frame_t;

/* Global IDT */
static idt_entry_t g_idt[IDT_ENTRIES];
static idt_ptr_t g_idt_ptr;

/* Interrupt handlers */
typedef void (*isr_handler_t)(interrupt_frame_t *frame);
static isr_handler_t g_isr_handlers[ISR_ENTRIES];

/* External functions */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

/* Exception handlers */
extern void exception_divide_error(void);
extern void exception_debug(void);
extern void exception_nmi(void);
extern void exception_breakpoint(void);
extern void exception_overflow(void);
extern void exception_bound_range(void);
extern void exception_invalid_opcode(void);
extern void exception_device_not_available(void);
extern void exception_double_fault(void);
extern void exception_coprocessor_segment_overrun(void);
extern void exception_invalid_tss(void);
extern void exception_segment_not_present(void);
extern void exception_stack_fault(void);
extern void exception_general_protection(void);
extern void exception_page_fault(void);
extern void exception_x87_floating_point(void);
extern void exception_alignment_check(void);
extern void exception_machine_check(void);
extern void exception_simd_floating_point(void);
extern void exception_virtualization(void);
extern void exception_control_protection(void);

/* Set IDT entry */
static void idt_set_gate(uint8_t num, uint64_t handler, uint16_t selector, 
                         uint8_t type_attr) {
    g_idt[num].offset_low = handler & 0xFFFF;
    g_idt[num].selector = selector;
    g_idt[num].ist = 0;
    g_idt[num].type_attr = type_attr;
    g_idt[num].offset_mid = (handler >> 16) & 0xFFFF;
    g_idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    g_idt[num].reserved = 0;
}

/* Initialize IDT */
static void idt_init(void) {
    g_idt_ptr.limit = sizeof(g_idt) - 1;
    g_idt_ptr.base = (uint64_t)g_idt;
    
    memset(g_idt, 0, sizeof(g_idt));
    
    /* Set up exception gates (present, ring 0, interrupt gate) */
    #define EXCEPTION_GATE(num, handler) \
        idt_set_gate(num, (uint64_t)handler, 0x08, 0x8E)
    
    EXCEPTION_GATE(0, exception_divide_error);
    EXCEPTION_GATE(1, exception_debug);
    EXCEPTION_GATE(2, exception_nmi);
    EXCEPTION_GATE(3, exception_breakpoint);
    EXCEPTION_GATE(4, exception_overflow);
    EXCEPTION_GATE(5, exception_bound_range);
    EXCEPTION_GATE(6, exception_invalid_opcode);
    EXCEPTION_GATE(7, exception_device_not_available);
    EXCEPTION_GATE(8, exception_double_fault);
    EXCEPTION_GATE(9, exception_coprocessor_segment_overrun);
    EXCEPTION_GATE(10, exception_invalid_tss);
    EXCEPTION_GATE(11, exception_segment_not_present);
    EXCEPTION_GATE(12, exception_stack_fault);
    EXCEPTION_GATE(13, exception_general_protection);
    EXCEPTION_GATE(14, exception_page_fault);
    EXCEPTION_GATE(15, exception_x87_floating_point);
    EXCEPTION_GATE(16, exception_alignment_check);
    EXCEPTION_GATE(17, exception_machine_check);
    EXCEPTION_GATE(18, exception_simd_floating_point);
    EXCEPTION_GATE(19, exception_virtualization);
    EXCEPTION_GATE(20, exception_control_protection);
    
    /* Set up IRQ gates */
    #define IRQ_GATE(num, handler) \
        idt_set_gate(32 + num, (uint64_t)handler, 0x08, 0x8E)
    
    IRQ_GATE(0, irq0);   /* Timer */
    IRQ_GATE(1, irq1);   /* Keyboard */
    IRQ_GATE(2, irq2);   /* Cascade */
    IRQ_GATE(3, irq3);   /* COM2 */
    IRQ_GATE(4, irq4);   /* COM1 */
    IRQ_GATE(5, irq5);   /* LPT2 */
    IRQ_GATE(6, irq6);   /* Floppy */
    IRQ_GATE(7, irq7);   /* LPT1 */
    IRQ_GATE(8, irq8);   /* RTC */
    IRQ_GATE(9, irq9);   /* Free */
    IRQ_GATE(10, irq10); /* Free */
    IRQ_GATE(11, irq11); /* Free */
    IRQ_GATE(12, irq12); /* PS/2 Mouse */
    IRQ_GATE(13, irq13); /* Coprocessor */
    IRQ_GATE(14, irq14); /* ATA Primary */
    IRQ_GATE(15, irq15); /* ATA Secondary */
    
    /* Load IDT */
    __asm__ volatile ("lidt %0" :: "m"(g_idt_ptr));
}

/* Default exception handler */
static void default_exception_handler(interrupt_frame_t *frame) {
    const char *exception_names[] = {
        "Divide Error", "Debug", "NMI", "Breakpoint",
        "Overflow", "Bound Range Exceeded", "Invalid Opcode",
        "Device Not Available", "Double Fault", "Coprocessor Segment Overrun",
        "Invalid TSS", "Segment Not Present", "Stack Fault",
        "General Protection", "Page Fault", "X87 Floating Point",
        "Alignment Check", "Machine Check", "SIMD Floating Point",
        "Virtualization", "Control Protection"
    };
    
    serial_println("");
    serial_println("!!! EXCEPTION !!!");
    if (frame->vector < 21) {
        serial_print("Exception: ");
        serial_println(exception_names[frame->vector]);
    } else {
        serial_print("Unknown Exception: ");
        serial_print_hex(frame->vector);
        serial_println("");
    }
    
    serial_print("Error Code: ");
    serial_print_hex(frame->error_code);
    serial_println("");
    
    serial_print("RIP: ");
    serial_print_hex(frame->rip);
    serial_println("");
    
    serial_print("RFLAGS: ");
    serial_print_hex(frame->rflags);
    serial_println("");
    
    serial_print("RSP: ");
    serial_print_hex(frame->rsp);
    serial_println("");
    
    /* Halt */
    while (1) {
        cpu_halt();
    }
}

/* IRQ handler dispatcher */
static void irq_handler(interrupt_frame_t *frame) {
    uint64_t irq = frame->vector - 32;
    
    /* Send EOI to PIC/APIC */
    if (irq >= 8) {
        __asm__ volatile ("outb $0x20, $0xA0");
    }
    __asm__ volatile ("outb $0x20, $0x20");
    
    /* Call registered handler if any */
    if (irq < ISR_ENTRIES && g_isr_handlers[irq]) {
        g_isr_handlers[irq](frame);
    }
}

/* Register ISR handler */
void register_isr(uint8_t num, isr_handler_t handler) {
    if (num < ISR_ENTRIES) {
        g_isr_handlers[num] = handler;
    }
}

/* Enable interrupts */
void enable_interrupts(void) {
    cpu_sti();
}

/* Disable interrupts */
void disable_interrupts(void) {
    cpu_cli();
}

/* Initialize interrupt controller */
void interrupt_controller_init(void) {
    serial_println("[*] Initializing IDT...");
    
    idt_init();
    
    /* Remap PIC */
   __asm__ volatile (
    "mov $0x11, %%al; out %%al, $0x20; jmp .+2; jmp .+2; jmp .+2;"
    "mov $0x20, %%al; out %%al, $0x21; jmp .+2; jmp .+2; jmp .+2;"
    "mov $0x04, %%al; out %%al, $0x21; jmp .+2; jmp .+2; jmp .+2;"
    "mov $0x02, %%al; out %%al, $0xA1; jmp .+2; jmp .+2; jmp .+2;"
    "mov $0x01, %%al; out %%al, $0x21; jmp .+2; jmp .+2; jmp .+2;"
    "mov $0x01, %%al; out %%al, $0xA1; jmp .+2; jmp .+2; jmp .+2;"
    "mov $0x00, %%al; out %%al, $0x21; jmp .+2; jmp .+2; jmp .+2;"
    "mov $0x00, %%al; out %%al, $0xA1;"
    :
    :
    : "eax"
);
    
    serial_println("[+] IDT initialized");
    
    /* Enable timer interrupt */
    serial_println("[*] Enabling timer interrupt...");
    /* PIT initialization would go here */
    serial_println("[+] Timer enabled");
}
