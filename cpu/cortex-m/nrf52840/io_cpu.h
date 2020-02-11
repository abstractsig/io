/*
 *
 * io representation of a nrf52840 cpu
 *
 */
#ifndef io_cpu_H_
#define io_cpu_H_
#include <io_core.h>
#include <nrf52840.h>
#include <nrf52_to_nrf52840.h>
#include <nrf52840_bitfields.h>
#include <nrf52840_peripherals.h>

#define NRF52840_IO_CPU_STRUCT_MEMBERS \
	IO_STRUCT_MEMBERS				\
	io_value_memory_t *vm;\
	io_byte_memory_t *bm;\
	uint32_t in_event_thread;\
	/**/

typedef struct PACK_STRUCTURE nrf52840_io {
	NRF52840_IO_CPU_STRUCT_MEMBERS
} nrf52840_io_t;

void initialise_cpu_io (io_t*);


#define NUMBER_OF_ARM_INTERRUPT_VECTORS	16L
#define NUMBER_OF_NRF_INTERRUPT_VECTORS	47L
#define NUMBER_OF_INTERRUPT_VECTORS	(NUMBER_OF_ARM_INTERRUPT_VECTORS + NUMBER_OF_NRF_INTERRUPT_VECTORS)

#define ENABLE_INTERRUPTS	\
	do {	\
		__DMB();	\
		__enable_irq();	\
	} while (0)

#define DISABLE_INTERRUPTS	\
	do {	\
		__disable_irq();	\
		__DMB();	\
	} while (0)


#define EVENT_THREAD_INTERRUPT		SWI0_EGU0_IRQn
#define SET_EVENT_PENDING				NVIC_SetPendingIRQ (EVENT_THREAD_INTERRUPT)


//-----------------------------------------------------------------------------
//
// nrf52840 Implementtaion
//
//-----------------------------------------------------------------------------
#ifdef IMPLEMENT_IO_CPU

static io_byte_memory_t*
nrf52_io_get_byte_memory (io_t *io) {
	nrf52840_io_t *this = (nrf52840_io_t*) io;
	return this->bm;
}

static io_value_memory_t*
nrf52_io_get_stm (io_t *io) {
	nrf52840_io_t *this = (nrf52840_io_t*) io;
	return this->vm;
}

void
add_io_implementation_cpu_methods (io_implementation_t *io_i) {
	add_io_implementation_core_methods (io_i);

	io_i->get_byte_memory = nrf52_io_get_byte_memory;
	io_i->get_short_term_value_memory = nrf52_io_get_stm;

/*
	io_i->do_gc = NULL;
	io_i->get_core_clock = NULL;
	io_i->get_random_u32 = NULL;
	io_i->get_socket = NULL;
	io_i->dequeue_event = NULL;
	io_i->enqueue_event = NULL;
	io_i->next_event = NULL;
	io_i->in_event_thread = NULL;
	io_i->signal_event_pending = NULL;
	io_i->wait_for_event = NULL;
	io_i->wait_for_all_events = NULL;
	io_i->enter_critical_section = NULL;
	io_i->exit_critical_section = NULL;
	io_i->register_interrupt_handler = NULL;
	io_i->unregister_interrupt_handler = NULL;
	io_i->log = NULL;
	io_i->panic = NULL;
*/
}

static io_byte_memory_t heap_byte_memory;
static io_byte_memory_t umm_value_memory;
static umm_io_value_memory_t short_term_values;

void
initialise_cpu_io (io_t *io) {
	nrf52840_io_t *this = (nrf52840_io_t*) io;

	this->bm = &heap_byte_memory;
	this->vm = (io_value_memory_t*) &short_term_values;

	short_term_values.io = io;
	initialise_io_byte_memory (io,&heap_byte_memory);
	initialise_io_byte_memory (io,&umm_value_memory);

}

static void apply_nrf_cpu_errata (void);
static io_interrupt_handler_t cpu_interrupts[NUMBER_OF_INTERRUPT_VECTORS];

static void
null_interrupt_handler (void *w) {
	while(1);
}

static void
initialise_ram_interrupt_vectors (void) {
	io_interrupt_handler_t *i = cpu_interrupts;
	io_interrupt_handler_t *e = i + NUMBER_OF_INTERRUPT_VECTORS;
	while (i < e) {
		i->action = null_interrupt_handler;
		i->user_value = NULL;
		i++;
	}
}

static void
handle_io_cpu_interrupt (void) {
	io_interrupt_handler_t const *interrupt = &cpu_interrupts[
		SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk
	];
	interrupt->action(interrupt->user_value);
}

static void
initialise_c_runtime (void) {
	extern uint32_t ld_start_of_sdata_in_flash;
	extern uint32_t ld_start_of_sdata_in_ram,ld_end_of_sdata_in_ram;
	extern uint32_t ld_start_of_bss,ld_end_of_bss;

	uint32_t *src = &ld_start_of_sdata_in_flash;
	uint32_t *dest = &ld_start_of_sdata_in_ram;

	while(dest < &ld_end_of_sdata_in_ram) *dest++ = *src++;
	dest = &ld_start_of_bss;
	while(dest < &ld_end_of_bss) *dest++ = 0;

	// fill stack/heap region of RAM with a pattern
	extern uint32_t ld_end_of_static_ram_allocations;
	uint32_t *end = (uint32_t*) __get_MSP();
	dest = &ld_end_of_static_ram_allocations;
	while (dest < end) {
		*dest++ = 0xdeadc0de;
	}
	
	initialise_ram_interrupt_vectors ();
}

int main(void);
void
nrf52_core_reset (void) {
	initialise_c_runtime ();
	apply_nrf_cpu_errata ();

	SCB->CPACR |= (
			(3UL << 10*2)	// set CP10 Full Access
		|	(3UL << 11*2)	// set CP11 Full Access 
	);

	main ();
	while (1);
}

extern uint32_t ld_top_of_c_stack;
__attribute__ ((section(".isr_vector")))
const void* s_flash_vector_table[NUMBER_OF_INTERRUPT_VECTORS] = {
	&ld_top_of_c_stack,
	nrf52_core_reset,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
};


static bool errata_36(void);
static bool errata_66(void);
static bool errata_98(void);
static bool errata_103(void);
static bool errata_115(void);
static bool errata_120(void);
static bool errata_136(void);

void
apply_nrf_cpu_errata (void) {
	/* Enable SWO trace functionality. If ENABLE_SWO is not defined, SWO pin will be used as GPIO (see Product
	Specification to see which one). */
	#if defined (ENABLE_SWO)
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	NRF_CLOCK->TRACECONFIG |= CLOCK_TRACECONFIG_TRACEMUX_Serial << CLOCK_TRACECONFIG_TRACEMUX_Pos;
	NRF_P1->PIN_CNF[0] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
	#endif

    /* Enable Trace functionality. If ENABLE_TRACE is not defined, TRACE pins will be used as GPIOs (see Product
       Specification to see which ones). */
    #if defined (ENABLE_TRACE)
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        NRF_CLOCK->TRACECONFIG |= CLOCK_TRACECONFIG_TRACEMUX_Parallel << CLOCK_TRACECONFIG_TRACEMUX_Pos;
        NRF_P0->PIN_CNF[7]  = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
        NRF_P1->PIN_CNF[0]  = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
        NRF_P0->PIN_CNF[12] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
        NRF_P0->PIN_CNF[11] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
        NRF_P1->PIN_CNF[9]  = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
    #endif
    
    /* Workaround for Errata 36 "CLOCK: Some registers are not reset when expected" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/  */
    if (errata_36()){
        NRF_CLOCK->EVENTS_DONE = 0;
        NRF_CLOCK->EVENTS_CTTO = 0;
        NRF_CLOCK->CTIV = 0;
    }
    
    /* Workaround for Errata 66 "TEMP: Linearity specification not met with default settings" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/  */
    if (errata_66()){
        NRF_TEMP->A0 = NRF_FICR->TEMP.A0;
        NRF_TEMP->A1 = NRF_FICR->TEMP.A1;
        NRF_TEMP->A2 = NRF_FICR->TEMP.A2;
        NRF_TEMP->A3 = NRF_FICR->TEMP.A3;
        NRF_TEMP->A4 = NRF_FICR->TEMP.A4;
        NRF_TEMP->A5 = NRF_FICR->TEMP.A5;
        NRF_TEMP->B0 = NRF_FICR->TEMP.B0;
        NRF_TEMP->B1 = NRF_FICR->TEMP.B1;
        NRF_TEMP->B2 = NRF_FICR->TEMP.B2;
        NRF_TEMP->B3 = NRF_FICR->TEMP.B3;
        NRF_TEMP->B4 = NRF_FICR->TEMP.B4;
        NRF_TEMP->B5 = NRF_FICR->TEMP.B5;
        NRF_TEMP->T0 = NRF_FICR->TEMP.T0;
        NRF_TEMP->T1 = NRF_FICR->TEMP.T1;
        NRF_TEMP->T2 = NRF_FICR->TEMP.T2;
        NRF_TEMP->T3 = NRF_FICR->TEMP.T3;
        NRF_TEMP->T4 = NRF_FICR->TEMP.T4;
    }
    
    /* Workaround for Errata 98 "NFCT: Not able to communicate with the peer" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/  */
    if (errata_98()){
        *(volatile uint32_t *)0x4000568Cul = 0x00038148ul;
    }
    
    /* Workaround for Errata 103 "CCM: Wrong reset value of CCM MAXPACKETSIZE" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/  */
    if (errata_103()){
        NRF_CCM->MAXPACKETSIZE = 0xFBul;
    }
    
    /* Workaround for Errata 115 "RAM: RAM content cannot be trusted upon waking up from System ON Idle or System OFF mode" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/  */
    if (errata_115()){
        *(volatile uint32_t *)0x40000EE4 = (*(volatile uint32_t *)0x40000EE4 & 0xFFFFFFF0) | (*(uint32_t *)0x10000258 & 0x0000000F);
    }
    
    /* Workaround for Errata 120 "QSPI: Data read or written is corrupted" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/  */
    if (errata_120()){
        *(volatile uint32_t *)0x40029640ul = 0x200ul;
    }
    
    /* Workaround for Errata 136 "System: Bits in RESETREAS are set when they should not be" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/  */
    if (errata_136()){
        if (NRF_POWER->RESETREAS & POWER_RESETREAS_RESETPIN_Msk){
            NRF_POWER->RESETREAS =  ~POWER_RESETREAS_RESETPIN_Msk;
        }
    }
    
    /* Enable the FPU if the compiler used floating point unit instructions. __FPU_USED is a MACRO defined by the
     * compiler. Since the FPU consumes energy, remember to disable FPU use in the compiler if floating point unit
     * operations are not used in your code. */
    #if (__FPU_USED == 1)
        SCB->CPACR |= (3UL << 20) | (3UL << 22);
        __DSB();
        __ISB();
    #endif

    /* Configure NFCT pins as GPIOs if NFCT is not to be used in your code. If CONFIG_NFCT_PINS_AS_GPIOS is not defined,
       two GPIOs (see Product Specification to see which ones) will be reserved for NFC and will not be available as
       normal GPIOs. */
    #if defined (CONFIG_NFCT_PINS_AS_GPIOS)
        if ((NRF_UICR->NFCPINS & UICR_NFCPINS_PROTECT_Msk) == (UICR_NFCPINS_PROTECT_NFC << UICR_NFCPINS_PROTECT_Pos)){
            NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
            while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
            NRF_UICR->NFCPINS &= ~UICR_NFCPINS_PROTECT_Msk;
            while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
            NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
            while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
            NVIC_SystemReset();
        }
    #endif

    /* Configure GPIO pads as pPin Reset pin if Pin Reset capabilities desired. If CONFIG_GPIO_AS_PINRESET is not
      defined, pin reset will not be available. One GPIO (see Product Specification to see which one) will then be
      reserved for PinReset and not available as normal GPIO. */
    #if defined (CONFIG_GPIO_AS_PINRESET)
        if (((NRF_UICR->PSELRESET[0] & UICR_PSELRESET_CONNECT_Msk) != (UICR_PSELRESET_CONNECT_Connected << UICR_PSELRESET_CONNECT_Pos)) ||
            ((NRF_UICR->PSELRESET[1] & UICR_PSELRESET_CONNECT_Msk) != (UICR_PSELRESET_CONNECT_Connected << UICR_PSELRESET_CONNECT_Pos))){
            NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
            while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
            NRF_UICR->PSELRESET[0] = 18;
            while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
            NRF_UICR->PSELRESET[1] = 18;
            while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
            NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
            while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
            NVIC_SystemReset();
        }
    #endif

}


static bool errata_36(void)
{
    if (*(uint32_t *)0x10000130ul == 0x8ul){
        if (*(uint32_t *)0x10000134ul == 0x0ul){
            return true;
        }
        if (*(uint32_t *)0x10000134ul == 0x1ul){
            return true;
        }
        if (*(uint32_t *)0x10000134ul == 0x2ul){
            return true;
        }
    }
    
    return true;
}


static bool errata_66(void)
{
    if (*(uint32_t *)0x10000130ul == 0x8ul){
        if (*(uint32_t *)0x10000134ul == 0x0ul){
            return true;
        }
        if (*(uint32_t *)0x10000134ul == 0x1ul){
            return true;
        }
        if (*(uint32_t *)0x10000134ul == 0x2ul){
            return true;
        }
    }
    
    return true;
}


static bool errata_98(void)
{
    if (*(uint32_t *)0x10000130ul == 0x8ul){
        if (*(uint32_t *)0x10000134ul == 0x0ul){
            return true;
        }
    }
    
    return false;
}


static bool errata_103(void)
{
    if (*(uint32_t *)0x10000130ul == 0x8ul){
        if (*(uint32_t *)0x10000134ul == 0x0ul){
            return true;
        }
    }
    
    return false;
}


static bool errata_115(void)
{
    if (*(uint32_t *)0x10000130ul == 0x8ul){
        if (*(uint32_t *)0x10000134ul == 0x0ul){
            return true;
        }
    }
    
    return false;
}


static bool errata_120(void)
{
    if (*(uint32_t *)0x10000130ul == 0x8ul){
        if (*(uint32_t *)0x10000134ul == 0x0ul){
            return true;
        }
    }
    
    return false;
}


static bool errata_136(void)
{
    if (*(uint32_t *)0x10000130ul == 0x8ul){
        if (*(uint32_t *)0x10000134ul == 0x0ul){
            return true;
        }
        if (*(uint32_t *)0x10000134ul == 0x1ul){
            return true;
        }
        if (*(uint32_t *)0x10000134ul == 0x2ul){
            return true;
        }
    }
    
    return true;
}
#endif /* IMPLEMENT_IO_CPU */
#ifdef IMPLEMENT_VERIFY_IO_CPU

#endif /* IMPLEMENT_VERIFY_IO_CPU */
#ifdef IMPLEMENT_VERIFY_IO_CORE
UNIT_SETUP(setup_io_cpu_unit_test) {
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_cpu_unit_test) {
}

void
io_cpu_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		#ifdef IMPLEMENT_VERIFY_IO_CPU
		#endif
		0
	};
	unit->name = "io cpu";
	unit->description = "io cpu unit test";
	unit->tests = tests;
	unit->setup = setup_io_cpu_unit_test;
	unit->teardown = teardown_io_cpu_unit_test;
}
#endif /* IMPLEMENT_VERIFY_IO_CORE */
#endif /* io_cpu_H_ */
/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2020 Gregor Bruce
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/

