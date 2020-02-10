/*
 *
 * io representation of a stm32f4xx cpu
 *
 */
#ifndef io_cpu_H_
#define io_cpu_H_
#include <io_core.h>
#include <stm32f4xx_peripherals.h>

//
// clocks
//

typedef struct PACK_STRUCTURE stm32f4_crystal_oscillator {
	IO_CPU_CLOCK_SOURCE_STRUCT_MEMBERS
	float64_t crystal_frequency;
} stm32f4_crystal_oscillator_t;

typedef struct PACK_STRUCTURE stm32f4_rc_oscillator {
	IO_CPU_CLOCK_SOURCE_STRUCT_MEMBERS
	float64_t frequency;
} stm32f4_rc_oscillator_t;

typedef struct PACK_STRUCTURE stm32f4_apb_clock {
	IO_CPU_CLOCK_FUNCTION_STRUCT_MEMBERS
	uint32_t divider;
} stm32f4_ahb_clock_t, stm32f4_apb_clock_t;

typedef struct PACK_STRUCTURE stm32f4_pll {
	IO_CPU_CLOCK_FUNCTION_STRUCT_MEMBERS

	uint32_t M;
	uint32_t N;
	uint32_t P;
	uint32_t Q;	// for USB

} stm32f4_pll_t;

typedef struct stm32f4_core_clock {
	IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS
} stm32f4_core_clock_t;

typedef struct stm32f4_peripheral_clock {
	IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS
	void (*enable_apb_clock) (void);
} stm32f4_peripheral_clock_t;

extern EVENT_DATA io_cpu_clock_implementation_t stm32f4_hsi_oscillator_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t stm32f4_hse_oscillator_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t stm32f4_pll_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t stm32f4_core_clock_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t stm32f4_ahb_clock_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t stm32f4_apb1_clock_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t stm32f4_apb2_clock_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t stm32f4_apb_peripheral_clock_implementation;

//
// pins
//
typedef union PACK_STRUCTURE {
	io_pin_t io;
	uint32_t u32;
	struct PACK_STRUCTURE {
		uint32_t port_id:4;
		uint32_t number:4;
		uint32_t alternate:4;
		uint32_t active_level:1;
		uint32_t initial_state:1;
		uint32_t function:2;
		uint32_t pull_mode:2;
		uint32_t speed:2;
		uint32_t output_type:1;
		uint32_t pull:2;
		uint32_t :9;
	} stm;
} stm32f4_io_pin_t;

struct stm_gpio_port {
	GPIO_TypeDef *registers;
	void (*enable_clock) (void);
};

extern const struct stm_gpio_port stm_gpio_ports[];

#define stm_gpio_pin_enable_port_clock(pin)		stm_gpio_ports[(pin).stm.port_id].enable_clock()
#define stm_gpio_pin_port(pin)					stm_gpio_ports[(pin).stm.port_id].registers
#define stm_gpio_pin_number(pin)				(pin).stm.number
#define stm_gpio_pin_speed(pin)					(pin).stm.speed
#define stm_gpio_pin_alternate(pin)				(pin).stm.alternate
#define stm_gpio_pin_pull_mode(pin)				(pin).stm.pull_mode
#define stm_gpio_pin_output_type(pin)			(pin).stm.output_type
#define stm_gpio_pin_pull(pin)					(pin).stm.pull
#define stm_gpio_pin_function(pin)				(pin).stm.function

enum {
	STM_GPIO_PORT_A = 0,
	STM_GPIO_PORT_B,
	STM_GPIO_PORT_C,
	STM_GPIO_PORT_D,
};

void configure_stm32f4_io_pin_alternate (stm32f4_io_pin_t);
void configure_stm32f4_io_pin (stm32f4_io_pin_t);


INLINE_FUNCTION void
stm32f4_pin_set_low (stm32f4_io_pin_t pin) {
	stm_gpio_pin_port(pin)->BSRRH = (1 << stm_gpio_pin_number(pin));
}

INLINE_FUNCTION void
stm32f4_pin_set_high (stm32f4_io_pin_t pin) {
	stm_gpio_pin_port(pin)->BSRRL = (1 << stm_gpio_pin_number(pin));
}

//
// sockets
//

typedef struct PACK_STRUCTURE stm32f4_uart {
	IO_SOCKET_STRUCT_MEMBERS

	io_t *io;
	io_encoding_implementation_t const *encoding;
	
	io_byte_pipe_t *rx_pipe;
	
	io_cpu_clock_pointer_t peripheral_bus_clock;
	USART_TypeDef* uart_registers;
	uint32_t baud_rate;
	IRQn_Type interrupt_number;
	stm32f4_io_pin_t tx_pin;
	stm32f4_io_pin_t rx_pin;

} stm32f4_uart_t;

extern EVENT_DATA io_socket_implementation_t stm32f4_uart_implementation;

#define USART_CR1_CLEAR_MASK ((uint16_t)(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS | USART_CR1_TE | USART_CR1_RE))


//
// STM32F4
//

#define STM32F4_IO_CPU_STRUCT_MEMBERS \
	IO_STRUCT_MEMBERS				\
	io_value_memory_t *vm;\
	io_byte_memory_t *bm;\
	uint32_t in_event_thread;\
	/**/

typedef struct PACK_STRUCTURE stm32f4xx_io {
	STM32F4_IO_CPU_STRUCT_MEMBERS
} stm32f4xx_io_t;

io_t*	initialise_cpu_io (stm32f4xx_io_t*);
void 	io_device_add_io_methods (io_implementation_t*);
void	microsecond_delay (uint32_t us);

void const* get_flash_sector_address(uint32_t);
bool flash_write_callback (uint32_t,void (*) (void*,void*),void *);

typedef struct {
	uint8_t data[64];
	uint32_t datalen;
	uint64_t bitlen;
	uint32_t state[8];
} sha256_context_t;

void sha256_init(sha256_context_t *ctx);
void sha256_update(sha256_context_t *ctx, const uint8_t data[], size_t len);
void sha256_final(sha256_context_t *ctx, uint8_t hash[]);

//
// STM32F4 io pins
//

typedef enum {
  ANALOG_CHANNEL_0 = 0,
  ANALOG_CHANNEL_1,
  ANALOG_CHANNEL_2,
  ANALOG_CHANNEL_3,
  ANALOG_CHANNEL_4,
  ANALOG_CHANNEL_5,
  ANALOG_CHANNEL_6,
  ANALOG_CHANNEL_7,
  ANALOG_CHANNEL_8,
  ANALOG_CHANNEL_9,
  ANALOG_CHANNEL_10,
  ANALOG_CHANNEL_11,
  ANALOG_CHANNEL_12,
  ANALOG_CHANNEL_13,
  ANALOG_CHANNEL_14,
  ANALOG_CHANNEL_15,
  ANALOG_CHANNEL_16,
  ANALOG_CHANNEL_17,
} analogue_channel_number_t;

//
// STM32F4 interrupts
//

#define ENABLE_INTERRUPTS			do {__DSB(); __enable_irq();} while (0);
#define DISABLE_INTERRUPTS			do {__disable_irq(); __DSB();} while (0);
#define EVENT_PRIORITY				((1 << __NVIC_PRIO_BITS) - 1)
#define EVENT_THREAD_INTERRUPT	PendSV_IRQn

#define NUMBER_OF_ARM_INTERRUPT_VECTORS	16L
#define NUMBER_OF_INTERRUPT_VECTORS (NUMBER_OF_ARM_INTERRUPT_VECTORS + NUMBER_OF_STM32F4_INTERRUPT_VECTORS)

typedef void (*io_cpu_interrupt_handler_t) (void*);

typedef struct PACK_STRUCTURE interrupt_handler {
	io_cpu_interrupt_handler_t handler;
	void *user_value;
} io_cpu_interrupt_t;

void	unhandled_cpu_interrupt (void*);
void	handle_io_cpu_interrupt (void);

#ifndef nmi_cpu_interrupt
# define nmi_cpu_interrupt	unhandled_cpu_interrupt
#endif
#ifndef hard_fault_cpu_interrupt
# define hard_fault_cpu_interrupt	unhandled_cpu_interrupt
#endif
#ifndef memory_manager_cpu_interrupt
# define memory_manager_cpu_interrupt	unhandled_cpu_interrupt
#endif
#ifndef bus_fault_cpu_interrupt
# define bus_fault_cpu_interrupt	unhandled_cpu_interrupt
#endif
#ifndef usage_fault_cpu_interrupt
# define usage_fault_cpu_interrupt	unhandled_cpu_interrupt
#endif
#ifndef service_cpu_interrupt
# define service_cpu_interrupt	unhandled_cpu_interrupt
#endif
#ifndef debug_monitor_cpu_interrupt
# define debug_monitor_cpu_interrupt	unhandled_cpu_interrupt
#endif
#ifndef pending_service_cpu_interrupt
# define pending_service_cpu_interrupt	unhandled_cpu_interrupt
#endif
#ifndef system_tick_cpu_interrupt
# define system_tick_cpu_interrupt	unhandled_cpu_interrupt
#endif
#ifndef uart1_io_cpu_interrupt
# define uart1_io_cpu_interrupt	unhandled_cpu_interrupt
#endif
#ifndef uart3_io_cpu_interrupt
# define uart3_io_cpu_interrupt	unhandled_cpu_interrupt
#endif

#ifndef timer3_io_cpu_interrupt
# define timer3_io_cpu_interrupt	unhandled_cpu_interrupt
#endif

//
// implementation
//
#ifdef IMPLEMENT_STM32F4_IO_CPU
//
// pins
//

static void
enable_gpio_port_a_clock (void) {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
}

static void
enable_gpio_port_b_clock (void) {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
}

static void
enable_gpio_port_c_clock (void) {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
}

static void
enable_gpio_port_d_clock (void) {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
}

const struct stm_gpio_port stm_gpio_ports[] = {
	{GPIOA,enable_gpio_port_a_clock},
	{GPIOB,enable_gpio_port_b_clock},
	{GPIOC,enable_gpio_port_c_clock},
	{GPIOD,enable_gpio_port_d_clock},
	{NULL}
};

void
configure_stm32f4_io_pin_alternate (stm32f4_io_pin_t pin) {
	GPIO_InitTypeDef GPIO_InitStructure;

	stm_gpio_pin_enable_port_clock(pin);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = stm_gpio_pin_output_type(pin);
	GPIO_InitStructure.GPIO_PuPd = stm_gpio_pin_pull (pin);
	GPIO_InitStructure.GPIO_Speed = stm_gpio_pin_speed (pin);
	GPIO_InitStructure.GPIO_Pin = (1 << stm_gpio_pin_number (pin));
	GPIO_Init(stm_gpio_pin_port (pin), &GPIO_InitStructure);

	GPIO_PinAFConfig (
		stm_gpio_pin_port (pin),
		stm_gpio_pin_number (pin),
		stm_gpio_pin_alternate(pin)
	);
}

void
configure_stm32f4_io_pin (stm32f4_io_pin_t pin) {
	GPIO_InitTypeDef GPIO_InitStructure;

	stm_gpio_pin_enable_port_clock(pin);

	GPIO_InitStructure.GPIO_Mode = stm_gpio_pin_function(pin);
	GPIO_InitStructure.GPIO_Pin = (1 << stm_gpio_pin_number (pin));
	GPIO_InitStructure.GPIO_OType = stm_gpio_pin_output_type(pin);
	GPIO_InitStructure.GPIO_Speed = stm_gpio_pin_speed (pin);
	GPIO_Init(stm_gpio_pin_port (pin), &GPIO_InitStructure);
}


//
// clocks
//
static float64_t
stm32f4_hsi_oscillator_get_frequency (io_cpu_clock_pointer_t this) {
	stm32f4_rc_oscillator_t const *c = (stm32f4_rc_oscillator_t const*) (
		io_cpu_clock_ro_pointer (this)
	);
	return c->frequency;
}

static bool
stm32f4_hsi_oscillator_start (io_cpu_clock_pointer_t this) {
	if (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == 0) {
		// HSION
		RCC->CR |= (uint32_t)0x00000001;
		while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == 0);
		return true;
	} else {
		return true;
	}
}

EVENT_DATA io_cpu_clock_implementation_t stm32f4_hsi_oscillator_implementation = {
	.specialisation_of = &io_cpu_clock_implementation,
	.get_frequency = stm32f4_hsi_oscillator_get_frequency,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = stm32f4_hsi_oscillator_start,
	.stop = NULL,
};

bool
stm32f4_clock_is_hsi (io_cpu_clock_pointer_t clock) {
	return io_cpu_clock_has_implementation (clock,&stm32f4_hsi_oscillator_implementation);
}

static float64_t
stm32f4_hse_oscillator_get_frequency (io_cpu_clock_pointer_t this) {
	stm32f4_crystal_oscillator_t const *c = (stm32f4_crystal_oscillator_t const*) (
		io_cpu_clock_ro_pointer (this)
	);
	return c->crystal_frequency;
}

static bool
stm32f4_hse_oscillator_start (io_cpu_clock_pointer_t this) {
	if (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == 0) {
		RCC_HSEConfig(RCC_HSE_ON);
		if (RCC_WaitForHSEStartUp () == SUCCESS) {
			return true;
		} else {
			return false;
		}
	} else {
		return true;
	}
}

EVENT_DATA io_cpu_clock_implementation_t stm32f4_hse_oscillator_implementation = {
	.specialisation_of = &io_cpu_clock_implementation,
	.get_frequency = stm32f4_hse_oscillator_get_frequency,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = stm32f4_hse_oscillator_start,
	.stop = NULL,
};

bool
stm32f4_clock_is_hse (io_cpu_clock_pointer_t clock) {
	return io_cpu_clock_has_implementation (clock,&stm32f4_hse_oscillator_implementation);
}

static float64_t
stm32f4_pll_oscillator_get_frequency (io_cpu_clock_pointer_t clock) {
	stm32f4_pll_t const *this = (
		(stm32f4_pll_t const*) io_cpu_clock_ro_pointer (clock)
	);
	float64_t f = io_cpu_clock_get_frequency (this->input);

	f = ((f / (float64_t) this->M) * (float64_t) this->N) / (float64_t) (this->P);

	return f;
}

static bool
stm32f4_pll_oscillator_start (io_cpu_clock_pointer_t clock) {
	if (io_cpu_dependant_clock_start_input (clock)) {
		stm32f4_pll_t const *this = (
			(stm32f4_pll_t const*) io_cpu_clock_ro_pointer (clock)
		);
		uint32_t source = 0;

		if (stm32f4_clock_is_hse (this->input)) {
			source = RCC_PLLCFGR_PLLSRC_HSE;
		} else if (stm32f4_clock_is_hsi (this->input)) {
			source = RCC_PLLCFGR_PLLSRC_HSI;
		} else {
			return false;
		}

		RCC->PLLCFGR = (
				this->M
			|	(this->N << 6)
			|	(((this->P >> 1) -1) << 16)
			| 	(source)
			|	(this->Q << 24)
		);
		RCC->CR |= RCC_CR_PLLON;

		while((RCC->CR & RCC_CR_PLLRDY) == 0);

		return true;
	} else {
		return false;
	}
}

EVENT_DATA io_cpu_clock_implementation_t stm32f4_pll_implementation = {
	.specialisation_of = &io_cpu_clock_implementation,
	.get_frequency = stm32f4_pll_oscillator_get_frequency,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = stm32f4_pll_oscillator_start,
	.stop = NULL,
};

static float64_t
stm32f4_apb_clock_get_frequency (io_cpu_clock_pointer_t clock) {
	stm32f4_apb_clock_t const *this = (
		(stm32f4_apb_clock_t const*) io_cpu_clock_ro_pointer (clock)
	);
	float64_t f = io_cpu_clock_get_frequency (this->input);
	return f / (float64_t) this->divider;
}

EVENT_DATA io_cpu_clock_implementation_t stm32f4_apb_clock_implementation = {
	.specialisation_of = &io_cpu_clock_implementation,
	.get_frequency = stm32f4_apb_clock_get_frequency,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = NULL,
	.stop = NULL,
};

void
stm32f4_enable_spi1_apb_clock (void) {
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
}

void
stm32f4_enable_uart1_apb_clock (void) {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
}

static bool
stm32f4_ahb_clock_start (io_cpu_clock_pointer_t clock) {
	if (io_cpu_dependant_clock_start_input (clock)) {
		stm32f4_ahb_clock_t const *this = (
			(stm32f4_ahb_clock_t const*) io_cpu_clock_ro_pointer (clock)
		);
		bool ok = true;

		switch (this->divider) {
			case 1:
				RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
			break;

			default:
				ok = false;
			break;
		}

		return ok;
	} else {
		return false;
	}
}

EVENT_DATA io_cpu_clock_implementation_t stm32f4_ahb_clock_implementation = {
	.specialisation_of = &stm32f4_apb_clock_implementation,
	.get_frequency = stm32f4_apb_clock_get_frequency,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = stm32f4_ahb_clock_start,
	.stop = NULL,
};

static bool
stm32f4_apb1_clock_start (io_cpu_clock_pointer_t clock) {
	if (io_cpu_dependant_clock_start_input (clock)) {
		stm32f4_apb_clock_t const *this = (
			(stm32f4_apb_clock_t const*) io_cpu_clock_ro_pointer (clock)
		);
		bool ok = true;

		// HCLK divider
		switch (this->divider) {
			case 1:
				RCC->CFGR |= RCC_CFGR_PPRE1_DIV1;
			break;
			case 4:
				RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;
			break;

			default:
				ok = false;
			break;
		}

		return ok;
	} else {
		return false;
	}
}

EVENT_DATA io_cpu_clock_implementation_t stm32f4_apb1_clock_implementation = {
	.specialisation_of = &stm32f4_apb_clock_implementation,
	.get_frequency = stm32f4_apb_clock_get_frequency,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = stm32f4_apb1_clock_start,
	.stop = NULL,
};

static bool
stm32f4_apb2_clock_start (io_cpu_clock_pointer_t clock) {
	if (io_cpu_dependant_clock_start_input (clock)) {
		stm32f4_apb_clock_t const *this = (
			(stm32f4_apb_clock_t const*) io_cpu_clock_ro_pointer (clock)
		);
		bool ok = true;

		// HCLK divider
		switch (this->divider) {
			case 1:
				RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;
			break;
			case 2:
				RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;
			break;
			case 4:
				RCC->CFGR |= RCC_CFGR_PPRE2_DIV4;
			break;

			default:
				ok = false;
			break;
		}

		return ok;
	} else {
		return false;
	}
}

EVENT_DATA io_cpu_clock_implementation_t stm32f4_apb2_clock_implementation = {
	.specialisation_of = &stm32f4_apb_clock_implementation,
	.get_frequency = stm32f4_apb_clock_get_frequency,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = stm32f4_apb2_clock_start,
	.stop = NULL,
};

static float64_t
stm32f4_core_clock_get_frequency (io_cpu_clock_pointer_t clock) {
	stm32f4_core_clock_t const *this = (stm32f4_core_clock_t const*) (
		io_cpu_clock_ro_pointer (clock)
	);
	return io_cpu_clock_get_frequency (this->input);
}

static bool
stm32f4_core_clock_start (io_cpu_clock_pointer_t clock) {
	if (io_cpu_dependant_clock_start_input (clock)) {
		stm32f4_core_clock_t const *this = (stm32f4_core_clock_t const*) (
			io_cpu_clock_ro_pointer (clock)
		);
		float64_t new_frequency = io_cpu_clock_get_frequency (this->input);

		if (io_math_compare_float64_ge (new_frequency,168000000.0)) {
			// based on frequency, set WS
			// wait states for 168MHz @ 3.3V
			FLASH->ACR |= FLASH_ACR_LATENCY_5WS;
		} else {
			// need to find the wait state table
		}

		/* Select the main PLL as system clock source */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
		RCC->CFGR |= RCC_CFGR_SW_PLL;

		/* Wait till the main PLL is used as system clock source */
		while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS ) != RCC_CFGR_SWS_PLL) {
		}

		return true;
	} else {
		return false;
	}
}

EVENT_DATA io_cpu_clock_implementation_t stm32f4_core_clock_implementation = {
	.specialisation_of = &io_cpu_clock_implementation,
	.get_frequency = stm32f4_core_clock_get_frequency,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = stm32f4_core_clock_start,
	.stop = NULL,
};

static bool
stm32f4_apb_peripheral_clock_start (io_cpu_clock_pointer_t clock) {
	if (io_dependant_cpu_clock_start (clock)) {
		stm32f4_peripheral_clock_t const *this = (
			(stm32f4_peripheral_clock_t const*) io_cpu_clock_ro_pointer (clock)
		);
		this->enable_apb_clock();
		return true;
	} else {
		return false;
	}
}

EVENT_DATA io_cpu_clock_implementation_t stm32f4_apb_peripheral_clock_implementation = {
	.specialisation_of = &io_cpu_clock_implementation,
	.get_frequency = io_dependant_cpu_clock_get_frequency,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = stm32f4_apb_peripheral_clock_start,
	.stop = NULL,
};

//
// STM32 UART
//

static void
stm32f4_uart_interrupt_handler (void *user_value) {
	stm32f4_uart_t *this = user_value;
	volatile uint16_t data = this->uart_registers->DR;

	if (io_pipe_put_byte (this->rx_pipe,data)) {
		io_enqueue_event (this->io,&this->rx_pipe->ev);
	}
}

static void
stm32f4_uart_initialise (io_socket_t *socket,io_t *io) {
	stm32f4_uart_t *this = (stm32f4_uart_t*) socket;
	this->io = io;

	this->rx_pipe = mk_io_byte_pipe (io_get_byte_memory(io),512);

	register_io_interrupt_handler (
		io,this->interrupt_number,stm32f4_uart_interrupt_handler,this
	);
}

static bool
stm32f4_uart_open (io_socket_t *socket) {
	stm32f4_uart_t *this = (stm32f4_uart_t*) socket;

	if (io_cpu_clock_start (this->peripheral_bus_clock)) {
		if ((this->uart_registers->CR1 & USART_CR1_UE) == 0) {
			float64_t freq = io_cpu_clock_get_frequency (this->peripheral_bus_clock);
			uint32_t baud,temp_reg;

			if ((this->uart_registers->CR1 & USART_CR1_OVER8) != 0) {
				freq /= (float64_t) (8 * 1 * (this->baud_rate));
			} else {
				freq /= (float64_t) (8 * 2 * (this->baud_rate));
			}

			baud = (uint32_t) (floor (freq));
			baud <<= 4;
			if ((this->uart_registers->CR1 & USART_CR1_OVER8) != 0) {
				baud += (uint32_t) ((freq - (floor (freq))) * 8.0);
			} else {
				baud += (uint32_t) ((freq - (floor (freq))) * 16.0);
			}

			this->uart_registers->BRR = (uint16_t) baud;

			configure_stm32f4_io_pin_alternate (this->tx_pin);
			configure_stm32f4_io_pin_alternate (this->rx_pin);

			temp_reg = this->uart_registers->CR2;
			temp_reg &= (uint32_t) ~((uint32_t)USART_CR2_STOP);
			temp_reg |= (uint32_t) USART_StopBits_1;
			this->uart_registers->CR2 = (uint16_t)temp_reg;

			temp_reg = this->uart_registers->CR1;
			temp_reg &= (uint32_t)~((uint32_t) USART_CR1_CLEAR_MASK);
			temp_reg |= (
					8
				|	USART_Parity_No
				|	USART_Mode_Rx
				|	USART_Mode_Tx
			);
			this->uart_registers->CR1 = (uint16_t)temp_reg;
			this->uart_registers->CR3 = 0;

			USART_ITConfig(this->uart_registers,USART_IT_RXNE, ENABLE);
			USART_Cmd(this->uart_registers, ENABLE);

			NVIC_SetPriority (this->interrupt_number,0);
			NVIC_EnableIRQ (this->interrupt_number);
		}
	}

	return true;
}

static void
stm32f4_uart_close (io_socket_t *socket) {
	// and then ....
}

static io_t*
stm32f4_uart_get_io (io_socket_t *socket) {
	stm32f4_uart_t *this = (stm32f4_uart_t*) socket;
	return this->io;
}

static io_encoding_t*
stm32f4_uart_new_message (io_socket_t *socket) {
	stm32f4_uart_t *this = (stm32f4_uart_t*) socket;
	return new_io_encoding (this->encoding,io_get_byte_memory(this->io));
}

static bool
stm32f4_uart_send_message_blocking (io_socket_t *socket,io_encoding_t *encoding) {
	if (is_io_binary_encoding (encoding)) {
		stm32f4_uart_t *this = (stm32f4_uart_t*) socket;
		const uint8_t *b,*e;

		io_encoding_get_ro_bytes (encoding,&b,&e);
		while (b < e) {
			while (!(this->uart_registers->SR & USART_SR_TXE));
			this->uart_registers->DR = *b++;
		}
		
		io_encoding_free(encoding);

		return true;
	} else {
		return false;
	}
}

static size_t
stm32f4_uart_mtu (io_socket_t const *socket) {
	return 1024;	//???
}

bool
stm32f4_uart_binds (io_socket_t *socket,io_event_t *rx) {
	stm32f4_uart_t *this = (stm32f4_uart_t*) socket;
	if (this->rx_pipe->ev.next_event == NULL) {
		this->rx_pipe->ev = *rx;
		return true;
	} else {
		return false;
	}
}

static io_pipe_t*
stm32f4_uart_get_inward_pipe (io_socket_t *socket) {
	stm32f4_uart_t *this = (stm32f4_uart_t*) socket;
	return (io_pipe_t*) this->rx_pipe;
}

EVENT_DATA io_socket_implementation_t stm32f4_uart_implementation = {
	.specialisation_of = NULL,
	.initialise = stm32f4_uart_initialise,
	.free = NULL,
	.get_io = stm32f4_uart_get_io,
	.open = stm32f4_uart_open,
	.close = stm32f4_uart_close,
	.binds = stm32f4_uart_binds,
	.get_inward_pipe = stm32f4_uart_get_inward_pipe,
	.new_message = stm32f4_uart_new_message,
	.send_message = stm32f4_uart_send_message_blocking,
	.iterate_inner_sockets = NULL,
	.iterate_outer_sockets = NULL,
	.mtu = stm32f4_uart_mtu,
};

//
// STM32 ADC
//
static int
stm32f4_analog_read (ADC_TypeDef *ADCx,uint32_t channel_number, bool fastConversion) {
	static bool is_initialised = false;
	bool needs_init = false;

	if (!fastConversion) {
		if (ADCx == ADC1) {
	      if (!is_initialised) {
	        is_initialised = true;
	        needs_init = true;
	        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	      }
		}
	}
	if (needs_init) {
		ADC_CommonInitTypeDef ADC_CommonInitStructure;
		ADC_InitTypeDef ADC_InitStructure;

		ADC_CommonStructInit(&ADC_CommonInitStructure);
		ADC_CommonInit(&ADC_CommonInitStructure);

		ADC_StructInit(&ADC_InitStructure);
		// Preinit
		ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
		ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
		ADC_InitStructure.ADC_ScanConvMode = DISABLE;
		ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
		ADC_Init(ADCx, &ADC_InitStructure);

		// Enable the ADC
		ADC_Cmd(ADCx, ENABLE);
	}

	uint8_t sampleTime = fastConversion ? ADC_SampleTime_3Cycles : ADC_SampleTime_480Cycles;
	ADC_RegularChannelConfig(ADCx,channel_number, 1, sampleTime);
	ADC_SoftwareStartConv(ADCx);

	while (ADC_GetFlagStatus(ADCx, ADC_FLAG_EOC) == RESET);

	return (int)ADC_GetConversionValue(ADCx);
}

//
// to generate, repeatedly read the voltage reference and XOR
// it into a rotated number to get a random-ish result
//
static uint32_t
stm32f4_random_uint32 (io_t *io) {
	ADC_TempSensorVrefintCmd(ENABLE);
	// don't wait here. We want to be reading as the voltage reference
	// tries to start up, so make things as random as we can!
	int s;
	uint32_t v = 0;
	for (s = 0; s < 32; s++) {
		v = (v<<3) ^ (v>>29) ^ (unsigned int)stm32f4_analog_read(ADC1,ANALOG_CHANNEL_17, false);
	}
	ADC_TempSensorVrefintCmd(DISABLE);
	return v;
}

static void
stm32f4_log (io_t *io,char const *fmt,va_list va) {

}

static io_byte_memory_t*
stm32f4_io_get_byte_memory (io_t *io) {
	stm32f4xx_io_t *this = (stm32f4xx_io_t*) io;
	return this->bm;
}

static io_value_memory_t*
stm32f4_io_get_stm (io_t *io) {
	stm32f4xx_io_t *this = (stm32f4xx_io_t*) io;
	return this->vm;
}

static void
stm32f4_panic (io_t *io,int code) {
	DISABLE_INTERRUPTS;
	while (1);
}

static void
stm32f4_wait_for_event (io_t *io) {
	__WFI();
}

static void
wait_for_all_events (io_t *io) {
	io_event_t *event;
	io_alarm_t *alarm;
	do {
		ENTER_CRITICAL_SECTION(io);
		event = io->events;
		alarm = io->alarms;
		EXIT_CRITICAL_SECTION(io);
	} while (
			event != &s_null_io_event
		&&	alarm != &s_null_io_alarm
	);
}

static bool
stm32f4_is_in_event_thread (io_t *io) {
	return ((stm32f4xx_io_t*) io)->in_event_thread;
}

static bool
stm32f4_enter_critical_section (io_t *env) {
	uint32_t interrupts_are_enabled = !(__get_PRIMASK() & 0x1);
	DISABLE_INTERRUPTS;
	return interrupts_are_enabled;
}

static void
stm32f4_exit_critical_section (io_t *io,bool were_enabled) {
	if (were_enabled) {
		ENABLE_INTERRUPTS;
	}
}

static void
signal_event_pending (io_t *io) {
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

static void
stm32f4_do_gc (io_t *io,int32_t count) {
	io_value_memory_do_gc (io_get_stm (io),count);
}

io_cpu_clock_pointer_t
stm32f4_get_core_clock (io_t *io) {
	extern EVENT_DATA stm32f4_core_clock_t cpu_core_clock;
	return IO_CPU_CLOCK(&cpu_core_clock);
}

static io_socket_t*
io_device_get_null_socket (io_t *io,int32_t h) {
	return NULL;
}

void
initialise_core_clock (io_t *io) {
	io_cpu_clock_pointer_t core_clock = io_get_core_clock(io);
	uint32_t cf = io_cpu_clock_get_frequency (core_clock);

	io_cpu_clock_start (core_clock);

	//
	// requires core clock frequency to be a multiple of 1MHz
	//
	SysTick->LOAD = cf/1000000UL;
	SysTick->LOAD -= 1UL;
	SysTick->VAL = 0UL;
	SysTick->CTRL = (
			SysTick_CTRL_CLKSOURCE_Msk
		|	SysTick_CTRL_ENABLE_Msk
	);
}


static void
tune_stm32f4_cpu (void) {
	#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
	// set CP10 and CP11 to full access
	SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));
	#endif

	/* Set HSION bit */
	RCC->CR |= (uint32_t)0x00000001;

	/* Reset CFGR register */
	RCC->CFGR = 0x00000000;

	/* Reset HSEON, CSSON and PLLON bits */
	RCC->CR &= (uint32_t)0xFEF6FFFF;

	/* Reset PLLCFGR register */
	RCC->PLLCFGR = 0x24003010;

	/* Reset HSEBYP bit */
	RCC->CR &= (uint32_t)0xFFFBFFFF;

	// Disable all interrupts
	RCC->CIR = 0x00000000;

	// enable instruction and data cache
	FLASH->ACR = (
			FLASH_ACR_ICEN
		|	FLASH_ACR_DCEN
	);

    // regulator voltage output Scale 1 for core clock up to 168 MHz
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_VOS;

}

static io_interrupt_handler_t cpu_interrupts[NUMBER_OF_INTERRUPT_VECTORS];
__attribute__ ((section(".ram_vector_table")))
uint32_t interrupt_vector_table[NUMBER_OF_INTERRUPT_VECTORS];

static void
null_interrupt_handler (void *w) {
	while(1);
}

static void	
register_interrupt_handler (
	io_t *io,int32_t number,io_interrupt_action_t handler,void *user_value
) {
	io_interrupt_handler_t *i = (
		cpu_interrupts + number + NUMBER_OF_ARM_INTERRUPT_VECTORS
	);
	i->action = handler;
	i->user_value = user_value;

	//
	// arm vextors are carried over for the original flash table
	//
	if (number < 0) {
		interrupt_vector_table[number + NUMBER_OF_ARM_INTERRUPT_VECTORS] = (
			(uint32_t) handle_io_cpu_interrupt
		);
	}
}
static io_byte_memory_t heap_byte_memory;
static io_byte_memory_t umm_value_memory;
static umm_io_value_memory_t stm;

static io_implementation_t io_i = {
	.get_byte_memory = stm32f4_io_get_byte_memory,
	.get_stm = stm32f4_io_get_stm,
	.do_gc = stm32f4_do_gc,
	.get_core_clock = stm32f4_get_core_clock,
	.get_random_u32 = stm32f4_random_uint32,
	.log = stm32f4_log,
	.panic = stm32f4_panic,
	.wait_for_event = stm32f4_wait_for_event,
	.wait_for_all_events = wait_for_all_events,
	.in_event_thread = stm32f4_is_in_event_thread,
	.enter_critical_section = stm32f4_enter_critical_section,
	.exit_critical_section = stm32f4_exit_critical_section,
	.register_interrupt_handler = register_interrupt_handler,
	.unregister_interrupt_handler = NULL,
	.signal_event_pending = signal_event_pending,
	.enqueue_event = enqueue_io_event,
	.dequeue_event = dequeue_io_event,
	.next_event = do_next_io_event,
	.get_socket = io_device_get_null_socket,
};

/*
 *-----------------------------------------------------------------------------
 *
 * get the io resources for the stm32f4
 *
 *-----------------------------------------------------------------------------
 */
io_t*
initialise_cpu_io (stm32f4xx_io_t *cpu) {
	io_t *io = (io_t*) cpu;

	// tune cpu ...
	NVIC_SetPriorityGrouping(0);
	SCB->VTOR = FLASH_BASE;

	io_device_add_io_methods (&io_i);
	
	cpu->implementation = &io_i;
	cpu->bm = &heap_byte_memory;
	cpu->vm = (io_value_memory_t*) &stm;
	cpu->in_event_thread = 0;

	initialise_io (io,&io_i);

	initialise_core_clock (io);

	stm.io = io;
	initialise_io_byte_memory (io,&heap_byte_memory);
	initialise_io_byte_memory (io,&umm_value_memory);

	return io;
}

void
handle_io_cpu_interrupt (void) {
	io_interrupt_handler_t const *interrupt = &cpu_interrupts[
		SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk
	];
	interrupt->action(interrupt->user_value);
}

static void
make_ram_interrupt_vectors (void) {
	extern const void* s_flash_vector_table[NUMBER_OF_ARM_INTERRUPT_VECTORS];

	uint32_t *dest = interrupt_vector_table;
	uint32_t *end = dest + NUMBER_OF_ARM_INTERRUPT_VECTORS;
	uint32_t *src = (uint32_t*) s_flash_vector_table;
	
	while (dest < end) *dest++ = *src++;

	// fill remaining vectors with default handler
	end += NUMBER_OF_STM32F4_INTERRUPT_VECTORS;
	while (dest < end) *dest++ = (uint32_t) handle_io_cpu_interrupt;

	{
		io_interrupt_handler_t *i = cpu_interrupts;
		io_interrupt_handler_t *e = i + NUMBER_OF_INTERRUPT_VECTORS;
		while (i < e) {
			i->action = null_interrupt_handler;
			i->user_value = NULL;
			i++;
		}
	}

	SCB->VTOR = (unsigned int) interrupt_vector_table;
}

static void
initialise_c_runtime (void) {
	extern uint32_t ld_start_of_sdata_in_flash,ld_start_of_sdata_in_ram,ld_end_of_sdata_in_ram;
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
	
	make_ram_interrupt_vectors ();
}

//
// entry point on core reset (CPU starts on HSI @ 16MHz)
//
int main(void);
void
stm32f4_core_reset (void) {
	initialise_c_runtime();
	tune_stm32f4_cpu ();
	main ();
	while (1);
}

void
unhandled_cpu_interrupt (void *user_value) {
	while (1);
}

extern uint32_t ld_top_of_c_stack;
__attribute__ ((section(".isr_vector")))
const void* s_flash_vector_table[/* 16 + 89 */] = {
	&ld_top_of_c_stack,
	stm32f4_core_reset,	//
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

	handle_io_cpu_interrupt,				// Window Watchdog interrupt
	handle_io_cpu_interrupt,				// PVD through EXTI line detection interrupt
	handle_io_cpu_interrupt,				// Tamper and TimeStamp interrupts through the EXTI line
	handle_io_cpu_interrupt,				// RTC Wakeup interrupt through the EXTI line
	handle_io_cpu_interrupt,				// Reserved
	handle_io_cpu_interrupt,				// RCC global interrupt
	handle_io_cpu_interrupt,				// EXTI Line0 interrupt
	handle_io_cpu_interrupt,				// EXTI Line1 interrupt
	handle_io_cpu_interrupt,				// EXTI Line2 interrupt
	handle_io_cpu_interrupt,				// EXTI Line3 interrupt
	handle_io_cpu_interrupt,				// EXTI Line4 interrupt
	handle_io_cpu_interrupt,				// DMA1 Stream0 global interrupt
	handle_io_cpu_interrupt,				// DMA1 Stream1 global interrupt
	handle_io_cpu_interrupt,				// DMA1 Stream2 global interrupt
	handle_io_cpu_interrupt,				// DMA1 Stream3 global interrupt
	handle_io_cpu_interrupt,				// DMA1 Stream4 global interrupt
	handle_io_cpu_interrupt,				// DMA1 Stream5 global interrupt
	handle_io_cpu_interrupt,				// DMA1 Stream6 global interrupt
	handle_io_cpu_interrupt,				// ADC3 global interrupts
	handle_io_cpu_interrupt,				// CAN1 TX interrupts
	handle_io_cpu_interrupt,				// CAN1 RX0 interrupts
	handle_io_cpu_interrupt,				// CAN1 RX1 interrupts
	handle_io_cpu_interrupt,				// CAN1 SCE interrupt
	handle_io_cpu_interrupt,				// EXTI Line[9:5] interrupts
	handle_io_cpu_interrupt,				// TIM1 Break interrupt and TIM9 global interrupt
	handle_io_cpu_interrupt,				// TIM1 Update interrupt and TIM10 global interrupt
	handle_io_cpu_interrupt,				// TIM1 Trigger and Commutation interrupts and TIM11 global interrupt
	handle_io_cpu_interrupt,				// TIM1 Capture Compare interrupt
	handle_io_cpu_interrupt,				// TIM2 global interrupt
	handle_io_cpu_interrupt,				// TIM3 global interrupt
	handle_io_cpu_interrupt,				// TIM4 global interrupt
	handle_io_cpu_interrupt,				// I2C1 event interrupt
	handle_io_cpu_interrupt,				// I2C1 error interrupt
	handle_io_cpu_interrupt,				// I2C2 event interrupt
	handle_io_cpu_interrupt,				// I2C2 error interrupt
	handle_io_cpu_interrupt,				// SPI1 global interrupt
	handle_io_cpu_interrupt,				// SPI2 global interrupt
	handle_io_cpu_interrupt,				// USART1 global interrupt
	handle_io_cpu_interrupt,				// USART2 global interrupt
	handle_io_cpu_interrupt,				// USART3 global interrupt
	handle_io_cpu_interrupt,				// EXTI Line[15:10] interrupts
	handle_io_cpu_interrupt,				// RTC Alarms (A and B) through EXTI line interrupt
	handle_io_cpu_interrupt,				// USB On-The-Go FS Wakeup through EXTI line interrupt
	handle_io_cpu_interrupt,				// TIM8 Break interrupt and TIM12 global interrupt
	handle_io_cpu_interrupt,				// TIM8 Update interrupt and TIM13 global interrupt
	handle_io_cpu_interrupt,				// TIM8 Trigger and Commutation interrupts and TIM14 global interrupt
	handle_io_cpu_interrupt,				// TIM8 Capture Compare interrupt
	handle_io_cpu_interrupt,				// DMA1 Stream7 global interrupt
	handle_io_cpu_interrupt,				// FSMC global interrupt
	handle_io_cpu_interrupt,				// SDIO global interrupt
	handle_io_cpu_interrupt,				// TIM5 global interrupt
	handle_io_cpu_interrupt,				// SPI3 global interrupt
	handle_io_cpu_interrupt,				// UART4 global interrupt
	handle_io_cpu_interrupt,				// UART5 global interrupt
	handle_io_cpu_interrupt,				// TIM6 global interrupt, DAC1 and DAC2 underrun error interrupt
	handle_io_cpu_interrupt,				// TIM7 global interrupt
	handle_io_cpu_interrupt,				// DMA2 Stream0 global interrupt
	handle_io_cpu_interrupt,				// DMA2 Stream1 global interrupt
	handle_io_cpu_interrupt,				// DMA2 Stream2 global interrupt
	handle_io_cpu_interrupt,				// DMA2 Stream3 global interrupt
	handle_io_cpu_interrupt,				// DMA2 Stream4 global interrupt
	handle_io_cpu_interrupt,				// Ethernet global interrupt
	handle_io_cpu_interrupt,				// Ethernet Wakeup through EXTI line interrupt
	handle_io_cpu_interrupt,				// CAN2 TX interrupts
	handle_io_cpu_interrupt,				// CAN2 RX0 interrupts
	handle_io_cpu_interrupt,				// CAN2 RX1 interrupts
	handle_io_cpu_interrupt,				// CAN2 SCE interrupt
	handle_io_cpu_interrupt,				// USB On The Go FS global interrupt
	handle_io_cpu_interrupt,				// DMA2 Stream5 global interrupt
	handle_io_cpu_interrupt,				// DMA2 Stream6 global interrupt
	handle_io_cpu_interrupt,				// DMA2 Stream7 global interrupt
	handle_io_cpu_interrupt,				// USART6 global interrupt
	handle_io_cpu_interrupt,				// I2C3 event interrupt
	handle_io_cpu_interrupt,				// I2C3 error interrupt
	handle_io_cpu_interrupt,				// USB On The Go HS End Point 1 Out global interrupt
	handle_io_cpu_interrupt,				// USB On The Go HS End Point 1 In global interrupt
	handle_io_cpu_interrupt,				// USB On The Go HS Wakeup through EXTI interrupt
	handle_io_cpu_interrupt,				// USB On The Go HS global interrupt
	handle_io_cpu_interrupt,				// DCMI global interrupt
	handle_io_cpu_interrupt,				// CRYP crypto global interrupt
	handle_io_cpu_interrupt,				// Hash and Rng global interrupt
	handle_io_cpu_interrupt,				// Floating point interrupt
	handle_io_cpu_interrupt,				// Reserved
	handle_io_cpu_interrupt,				// Reserved
	handle_io_cpu_interrupt,				// Reserved
	handle_io_cpu_interrupt,				// Reserved
	handle_io_cpu_interrupt,				// Reserved
	handle_io_cpu_interrupt,				// Reserved
	handle_io_cpu_interrupt,				// LTDC global interrupt
	handle_io_cpu_interrupt,				// LTDC global error interrupt
};

static uint32_t g_us_delay;

/*
 *-----------------------------------------------------------------------------
 *
 * SysTick_Handler --
 *
 *-----------------------------------------------------------------------------
 */
void
SysTick_Handler (void) {
	if (g_us_delay) {
		g_us_delay--;
	} else {
		SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
	}
}

/*
 *-----------------------------------------------------------------------------
 *
 * microsecond_delay --
 *
 * will block for at least us microseconds
 *
 *-----------------------------------------------------------------------------
 */
void
microsecond_delay (uint32_t us) {
	g_us_delay = us + 1;
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	while (SysTick->CTRL & SysTick_CTRL_TICKINT_Msk) {
	}
}

static uint8_t ALLOCATE_ALIGN(8) UMM_SECTION_DESCRIPTOR
heap_byte_memory_bytes[UMM_GLOBAL_HEAP_SIZE];
static io_byte_memory_t
heap_byte_memory = {
	.heap = (umm_block_t*) heap_byte_memory_bytes,
	.number_of_blocks = (UMM_GLOBAL_HEAP_SIZE / sizeof(umm_block_t)),
};

static uint8_t ALLOCATE_ALIGN(8) UMM_SECTION_DESCRIPTOR
umm_value_memory_bytes[UMM_VALUE_HEAP_SIZE];
static io_byte_memory_t
umm_value_memory = {
	.heap = (umm_block_t*) umm_value_memory_bytes,
	.number_of_blocks = (UMM_VALUE_HEAP_SIZE / sizeof(umm_block_t)),
};

static umm_io_value_memory_t stm = {
	.implementation = &umm_value_memory_implementation,
	.id_ = STVM,
	.bm = &umm_value_memory,
};

//
// success registered value memories
//
io_value_memory_t*
io_get_value_memory_by_id (uint32_t id) {
	if (id == STVM) {
		return (io_value_memory_t*) &stm;
	} else {
		return NULL;
	}
}

#define FLASH_WAIT_LIMIT 		(30000000*20)
#define SECTOR_SIZE	 			0x20000


static inline void
unlock_flash_control_register (void) {
	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xCDEF89AB;
}

bool
flash_wait_for_flash_ready(void) {
	uint32_t count = 0;
	while(FLASH->SR & FLASH_SR_BSY && count < FLASH_WAIT_LIMIT) count++;
	return count < FLASH_WAIT_LIMIT;
}

uint32_t s_sector_map[] = {
	0x08000000,
	0x08004000,
	0x08008000,
	0x0800C000,
	0x08010000,
	0x08020000,	// first 128k sector
	0x08040000,
	0x08060000,
	0x08080000,
	0x080A0000,
	0x080C0000,
	0x080E0000,
};

uint32_t
internal_flash_base_to_sector(uint32_t base) {
	uint32_t s = 0;
	while(s < SIZEOF(s_sector_map)) {
		if (s_sector_map[s] == base) break;
		s++;
	}
	return s;
}

//
// 128k sectors, numbers 5 through 11
//
bool
flash_erase_sector(uint32_t sector) {
	// wait until flash is not busy, can this fail?
	flash_wait_for_flash_ready();

	unlock_flash_control_register();

	FLASH->CR |= (2 << 8);	//set PSIZE to x32
	FLASH->CR |= FLASH_CR_SER;
	FLASH->CR = (FLASH->CR & ~(0x0f << 3)) + ((0x0f & sector) << 3);
	FLASH->CR |= FLASH_CR_STRT;

	flash_wait_for_flash_ready();
	FLASH->CR &= ~FLASH_CR_SER;
	FLASH->CR &= ~(3 << 8);	//set PSIZE to x8
	FLASH->CR |= FLASH_CR_LOCK;

	return 1;
}

//
// write to flash
//
bool
flash_write_callback (uint32_t sector,void (*cb) (void*,void*),void *user_value) {
	if (!(FLASH->CR & FLASH_CR_PG)) {
		void *flash = (void*) (s_sector_map[sector]);

		flash_erase_sector(sector);

		// wait until flash is not busy
		while (FLASH->SR & FLASH_SR_BSY);

		unlock_flash_control_register();
		FLASH->CR |= FLASH_CR_PG;	// enable write

		cb(flash,user_value);

		while (FLASH->SR & FLASH_SR_BSY);
		FLASH->CR &= ~FLASH_CR_PG;
		FLASH->CR |= FLASH_CR_LOCK;

		return true;
	} else {
		return false;
	}
}

void const*
get_flash_sector_address (uint32_t sector) {
	return (void const*) s_sector_map[sector];
}

/****************************** MACROS ******************************/
#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

/**************************** VARIABLES *****************************/
typedef unsigned int  WORD;             // 32-bit word, change to "long" for 16-bit machines

static const WORD k[64] = {
	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

/*********************** FUNCTION DEFINITIONS ***********************/
void sha256_transform(sha256_context_t *ctx, const uint8_t data[])
{
	WORD a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

	for (i = 0, j = 0; i < 16; ++i, j += 4)
		m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
	for ( ; i < 64; ++i)
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for (i = 0; i < 64; ++i) {
		t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a,b,c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}

void sha256_init(sha256_context_t *ctx)
{
	ctx->datalen = 0;
	ctx->bitlen = 0;
	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
}

void sha256_update(sha256_context_t *ctx, const uint8_t data[], size_t len)
{
	WORD i;

	for (i = 0; i < len; ++i) {
		ctx->data[ctx->datalen] = data[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			sha256_transform(ctx, ctx->data);
			ctx->bitlen += 512;
			ctx->datalen = 0;
		}
	}
}

void sha256_final(sha256_context_t *ctx, uint8_t hash[])
{
	WORD i;

	i = ctx->datalen;

	// Pad whatever data is left in the buffer.
	if (ctx->datalen < 56) {
		ctx->data[i++] = 0x80;
		while (i < 56)
			ctx->data[i++] = 0x00;
	}
	else {
		ctx->data[i++] = 0x80;
		while (i < 64)
			ctx->data[i++] = 0x00;
		sha256_transform(ctx, ctx->data);
		memset(ctx->data, 0, 56);
	}

	// Append to the padding the total message's length in bits and transform.
	ctx->bitlen += ctx->datalen * 8;
	ctx->data[63] = ctx->bitlen;
	ctx->data[62] = ctx->bitlen >> 8;
	ctx->data[61] = ctx->bitlen >> 16;
	ctx->data[60] = ctx->bitlen >> 24;
	ctx->data[59] = ctx->bitlen >> 32;
	ctx->data[58] = ctx->bitlen >> 40;
	ctx->data[57] = ctx->bitlen >> 48;
	ctx->data[56] = ctx->bitlen >> 56;
	sha256_transform(ctx, ctx->data);

	// Since this implementation uses little endian byte ordering and SHA uses big endian,
	// reverse all the bytes when copying the final state to the output hash.
	for (i = 0; i < 4; ++i) {
		hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
	}
}
#endif /* IMPLEMENT_STM32F4_IO_CPU */
#ifdef VERIFY_IO_CPU
#include <verify_io.h>

#endif /* VERIFY_IO_CPU */
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
