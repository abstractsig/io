/*
 *
 * Verify Io
 *
 * A unit test library for testing io
 *
 * LICENSE
 * See end of file for license terms.
 *
 * USAGE
 * Include this file in whatever places need to refer to it.
 * In one C-source file define IMPLEMENT_VERIFY_IO_CORE_GRAPHICS.
 *
 */
#ifndef io_verify_core_graphics_H_
#define io_verify_core_graphics_H_
#include <io_verify.h>
#include <io_graphics.h>

void	run_ut_io_core_graphics (V_runner_t*);

#ifdef IMPLEMENT_VERIFY_IO_CORE_GRAPHICS
TEST_BEGIN(test_io_graphics_command_line_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;
	io_graphics_command_stack_t *stack;
	
	io_byte_memory_get_info (bm,&bm_begin);

	stack = mk_io_graphics_command_stack (bm,5);

	VERIFY (
		io_graphics_command_stack_begin(stack) == io_graphics_command_stack_end (stack),
		NULL
	);
  
	VERIFY (
		io_graphics_stack_append_line (
			stack, def_i32_point(0,0), def_i32_point(1,0)
		),
		NULL
	);
	
	io_character_t text[] = {'a','b','c'};
	VERIFY (
		io_graphics_stack_append_text (
			stack,text,SIZEOF(text),def_i32_point(0,0)
		),
		NULL
	);

	free_io_graphics_command_stack (stack);

	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

UNIT_SETUP(setup_io_graphics_commands_unit_test) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_byte_memory_get_info (bm,TEST_MEMORY_INFO);
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_graphics_commands_unit_test) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_end;
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == TEST_MEMORY_INFO->used_bytes,NULL);
}

void
io_graphics_commands_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_io_graphics_command_line_1,
		0
	};
	unit->name = "io graphics commands";
	unit->description = "io graphics commands unit test";
	unit->tests = tests;
	unit->setup = setup_io_graphics_commands_unit_test;
	unit->teardown = teardown_io_graphics_commands_unit_test;
}


#define TEST_IO_GRAPHICS_CONTEXT_PIXEL_WIDTH		64
#define TEST_IO_GRAPHICS_CONTEXT_PIXEL_HEIGHT	48

static uint32_t
test_gfx_get_height_in_pixels (io_graphics_context_t *gfx) {
	return TEST_IO_GRAPHICS_CONTEXT_PIXEL_HEIGHT;
}

static uint32_t
test_gfx_get_width_in_pixels (io_graphics_context_t *gfx) {
	return TEST_IO_GRAPHICS_CONTEXT_PIXEL_WIDTH;
}

static bool test_gfx_get_pixel (io_graphics_context_t *gfx,io_i32_point_t at,io_pixel_t *px);
static void free_test_io_graphics_graphics_context (io_graphics_context_t *gfx);
static void test_gfx_draw_pixel (io_graphics_context_t*,io_i32_point_t);
static void test_gfx_set_colour (io_graphics_context_t*,io_colour_t);
static void test_gfx_begin (io_graphics_context_t*);
static void test_gfx_run (io_graphics_context_t*);
static void test_gfx_render (io_graphics_context_t*);
static void test_gfx_fill_rectangle (io_graphics_context_t*,io_i32_point_t,io_i32_point_t);
static io_colour_t  test_gfx_get_drawing_colour (io_graphics_context_t*);
static void	test_gfx_set_gamma_correction (io_graphics_context_t*,io_graphics_float_t);
static io_graphics_float_t test_gfx_get_gamma_correction (io_graphics_context_t*);
static io_graphics_command_stack_t* test_io_graphics_graphics_context_get_command_stack (io_graphics_context_t*);

EVENT_DATA io_graphics_context_implementation_t test_io_graphics_graphics_context_implementation = {
	.free = free_test_io_graphics_graphics_context,
	.select_font_by_name = io_graphics_context_select_ttf_font_by_name,
	.set_drawing_colour = test_gfx_set_colour,
	.get_drawing_colour = test_gfx_get_drawing_colour,
	.fill = io_graphics_context_fill_with_colour,
	.fill_rectangle = test_gfx_fill_rectangle,
	.get_pixel = test_gfx_get_pixel,
	.get_command_stack = test_io_graphics_graphics_context_get_command_stack,
	.draw_pixel = test_gfx_draw_pixel,
	.draw_character = io_graphics_context_draw_character_with_current_font,
	.draw_ascii_text = io_graphics_context_draw_draw_ascii_text_base,
	.begin = test_gfx_begin,
	.run = test_gfx_run,
	.render = test_gfx_render,
	.get_pixel_height = test_gfx_get_height_in_pixels,
	.get_pixel_width = test_gfx_get_width_in_pixels,
	.set_gamma_correction = test_gfx_set_gamma_correction,
	.get_gamma_correction = test_gfx_get_gamma_correction,
};

typedef struct PACK_STRUCTURE test_io_graphics_graphics_context {
	IO_GRAPHICS_CONTEXT_STRUCT_MEMBERS

	io_graphics_command_stack_t *stack;
	
	io_colour_t current_drawing_colour;
	//
	// should use current_draw_colour to access a pixel array as a void*
	// so we can use uint16_t's
	//
	io_pixel_t pixels[TEST_IO_GRAPHICS_CONTEXT_PIXEL_WIDTH][TEST_IO_GRAPHICS_CONTEXT_PIXEL_HEIGHT];

} test_io_graphics_graphics_context_t;

static io_graphics_context_t*
mk_test_io_graphics_graphics_context (
	io_byte_memory_t *bm,io_colour_t drawing_colour
) {
	test_io_graphics_graphics_context_t *this = io_byte_memory_allocate (
		bm,sizeof(test_io_graphics_graphics_context_t)
	);

	if (this) {
		this->implementation = &test_io_graphics_graphics_context_implementation;
		initialise_io_graphics_context (
			(io_graphics_context_t*) this,bm,io_graphics_ttf_fonts
		);
		this->stack = mk_io_graphics_command_stack (bm,5);
		this->current_drawing_colour = drawing_colour;
		io_graphics_context_fill (
			(io_graphics_context_t*) this,
			io_colour_get_std_colour(this->current_drawing_colour,IO_COLOUR_BLACK)
		);
	}

	return (io_graphics_context_t*) this;
}

void
free_test_io_graphics_graphics_context (io_graphics_context_t *gfx) {
	test_io_graphics_graphics_context_t *this = (test_io_graphics_graphics_context_t*) gfx;
	free_io_graphics_command_stack (this->stack);
	free_io_graphics_font (gfx->current_font);
	io_byte_memory_free (gfx->bm,gfx);
}

static io_graphics_command_stack_t*
test_io_graphics_graphics_context_get_command_stack (io_graphics_context_t *gfx) {
	test_io_graphics_graphics_context_t *this = (test_io_graphics_graphics_context_t*) gfx;
	return this->stack;
}

static void
test_gfx_set_colour (io_graphics_context_t *gfx,io_colour_t colour) {
	test_io_graphics_graphics_context_t *this = (test_io_graphics_graphics_context_t*) gfx;
	this->current_drawing_colour = colour;
}

static io_colour_t
test_gfx_get_drawing_colour (io_graphics_context_t *gfx) {
	test_io_graphics_graphics_context_t *this = (test_io_graphics_graphics_context_t*) gfx;
	return this->current_drawing_colour;
}

static bool
test_gfx_get_pixel (io_graphics_context_t *gfx,io_i32_point_t at,io_pixel_t *px) {
	if (
			at.x < TEST_IO_GRAPHICS_CONTEXT_PIXEL_WIDTH
		&&	at.y < TEST_IO_GRAPHICS_CONTEXT_PIXEL_HEIGHT
	) {
		test_io_graphics_graphics_context_t *this = (test_io_graphics_graphics_context_t*) gfx;
		*px = this->pixels[at.x][at.y];
		return true;
	} else {
		return false;
	}
}

void
test_gfx_draw_pixel (io_graphics_context_t *gfx,io_i32_point_t at) {
	if (
			at.x >= 0 && at.x < TEST_IO_GRAPHICS_CONTEXT_PIXEL_WIDTH
		&&	at.y >= 0 && at.y < TEST_IO_GRAPHICS_CONTEXT_PIXEL_HEIGHT
	) {
		test_io_graphics_graphics_context_t *this = (test_io_graphics_graphics_context_t*) gfx;
		this->pixels[at.x][at.y] = io_colour_level (this->current_drawing_colour);
	}
}

static void
test_gfx_fill_rectangle (io_graphics_context_t *gfx,io_i32_point_t p1,io_i32_point_t p2) {
	if (io_points_not_equal(p1,p2)) {
		test_io_graphics_graphics_context_t *this = (test_io_graphics_graphics_context_t*) gfx;
		int dx = p2.x - p1.x;
		int dy = p2.y - p1.y;
		int ix = dx < 0 ? -1 : 1;
		int iy = dy < 0 ? -1 : 1;
		int xe = p1.x + dx + ix;
		int ye = p1.y + dy + iy;

		int y = p1.y;
		do {
			int x = p1.x;
			do {
				this->pixels[x][y] = io_colour_level (this->current_drawing_colour);
				x += ix;
			} while (x != xe);
			y += iy;
		} while (y != ye);
	}
}

static void
test_gfx_begin (io_graphics_context_t *gfx) {
	test_io_graphics_graphics_context_t *this = (test_io_graphics_graphics_context_t*) gfx;
	reset_io_graphics_command_stack (this->stack);
}

static void
test_gfx_run (io_graphics_context_t *gfx) {
	test_io_graphics_graphics_context_t *this = (test_io_graphics_graphics_context_t*) gfx;
	io_graphics_command_t **cursor = io_graphics_command_stack_begin(this->stack);
	
	while (cursor < io_graphics_command_stack_end(this->stack)) {
		run_io_graphics_command (*cursor++,gfx);
	}
}

static void
test_gfx_render (io_graphics_context_t *gfx) {
	test_io_graphics_graphics_context_t *this = (test_io_graphics_graphics_context_t*) gfx;
	io_t *io = gfx->bm->io;
	uint32_t w,h,x,y;

	w = io_graphics_context_get_width_in_pixels(gfx);
	h = io_graphics_context_get_height_in_pixels(gfx);

	io_printf (io,"\nrender w=%d, h=%d\n",w,h);
	for (y = 0; y < h; ++y) {
		for (x = 0; x < w; ++x) {
			io_pixel_t px = this->pixels[x][y];
			uint8_t colour = (
					(
							(uint16_t)io_pixel_colour_red(px)
						+	(uint16_t)io_pixel_colour_green(px)
						+	(uint16_t)io_pixel_colour_blue(px)
					)
				/	(uint16_t)3
			);
			io_printf (io,"%c"," .:ioVM@"[colour>>5]);
		}
		io_printf (io,"\n");
	}
}

static void
test_gfx_set_gamma_correction (io_graphics_context_t *gfx,io_graphics_float_t g) {
}

static io_graphics_float_t
test_gfx_get_gamma_correction (io_graphics_context_t *gfx) {
	return 0;
}

//
// monochrome comparison only
//
static bool
check_gfx_pattern (uint8_t *expect,uint32_t w,uint32_t h,io_graphics_context_t *gfx) {
	io_pixel_t px;
	uint32_t x,y;
	bool ok = true;

	for (y = 0; y < h && ok; y++) {
		for (x = 0; x < w && ok; x++) {
			io_i32_point_t pt = {x,y};
			ok &= io_graphics_context_get_pixel (gfx,pt,&px);
			if (ok) {
				bool a = ((px.all & 0x00FFFFFF) != 0);
				bool b = *expect++;

				ok &= !(a ^ b);
			}
		}
	}

	return ok;
}

TEST_BEGIN(test_io_graphics_float_1) {
	io_graphics_float_t a,b;

	a = double_to_io_graphics_float(0.0);
	b = double_to_io_graphics_float(0.0 + io_graphics_float_to_double(FLOAT_COMPARE_EPSILON)/2.0);

	VERIFY (io_graphics_float_compare (a,b) == 0,NULL);

	a = double_to_io_graphics_float(1.0);
	b = double_to_io_graphics_float(1.0 + io_graphics_float_to_double(FLOAT_COMPARE_EPSILON)*2.0);
	VERIFY (io_graphics_float_compare (a,b) == -1,NULL);
	VERIFY (io_graphics_float_compare (b,a) == 1,NULL);
}
TEST_END

TEST_BEGIN(test_io_graphics_colour_1) {
	io_colour_t a = {.mix = &io_colour_mix_8bit,.level = {0xffffffff}};
	io_colour_t b = {.mix = &io_colour_mix_4bit,.level = {0xff0f0f0f}};

	io_colour_t c = convert_io_colour (a,b);

	VERIFY (colours_are_equal (b,c),NULL);

	c = IO_8BIT_COLOUR_RED;
	VERIFY (io_colour_get_red_level (c) == 255,NULL);
	VERIFY (io_colour_get_green_level (c) == 0,NULL);
	VERIFY (io_colour_get_blue_level (c) == 0,NULL);

	c = IO_8BIT_COLOUR_GREEN;
	VERIFY (io_colour_get_red_level (c) == 0,NULL);
	VERIFY (io_colour_get_green_level (c) == 255,NULL);
	VERIFY (io_colour_get_blue_level (c) == 0,NULL);

	c = IO_8BIT_COLOUR_BLUE;
	VERIFY (io_colour_get_red_level (c) == 0,NULL);
	VERIFY (io_colour_get_green_level (c) == 0,NULL);
	VERIFY (io_colour_get_blue_level (c) == 255,NULL);

	c = IO_6BIT_COLOUR_RED;
	VERIFY (io_colour_get_red_level (c) == 63,NULL);
	VERIFY (io_colour_get_green_level (c) == 0,NULL);
	VERIFY (io_colour_get_blue_level (c) == 0,NULL);

	c = convert_io_colour (a,IO_6BIT_COLOUR_RED);
	VERIFY (io_colour_get_red_level (c) == 63,NULL);

	c = IO_4BIT_COLOUR_RED;
	VERIFY (io_colour_get_red_level (c) == 15,NULL);
	VERIFY (io_colour_get_green_level (c) == 0,NULL);
	VERIFY (io_colour_get_blue_level (c) == 0,NULL);

	c = convert_io_colour (a,IO_WHITE_2BIT);
	VERIFY (io_colour_get_red_level (c) == 3,NULL);
	
	VERIFY (io_colour_get_maximum_bit_depth (IO_WHITE_2BIT) == 2,NULL);
	VERIFY (io_colour_get_maximum_bit_depth (IO_WHITE_4BIT) == 4,NULL);
	VERIFY (io_colour_get_maximum_bit_depth (IO_WHITE_6BIT) == 6,NULL);
	VERIFY (io_colour_get_maximum_bit_depth (IO_WHITE_8BIT) == 8,NULL);
}
TEST_END

TEST_BEGIN(test_io_graphics_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;
	io_graphics_context_t *gfx;

	io_byte_memory_get_info (bm,&bm_begin);

	gfx = mk_test_io_graphics_graphics_context(bm,IO_BLACK_8BIT);
	if (VERIFY (gfx != NULL,NULL)) {
		io_pixel_t px;
		io_i32_point_t pt;

		io_colour_t bg = {};

		VERIFY (
				io_graphics_context_get_width_in_pixels(gfx) == TEST_IO_GRAPHICS_CONTEXT_PIXEL_WIDTH
			&&	io_graphics_context_get_height_in_pixels(gfx) == TEST_IO_GRAPHICS_CONTEXT_PIXEL_HEIGHT,
			NULL
		);

		pt.x = 0,pt.y = 0;

		VERIFY (io_graphics_context_get_pixel (gfx,pt,&px),NULL);
		VERIFY (
				io_pixel_colour_red(px) == 0
			&&	io_pixel_colour_green(px) == 0
			&&	io_pixel_colour_blue(px) == 0,NULL
		);

		io_pixel_colour_red(io_colour_level(bg)) = 100;
		io_pixel_colour_green(io_colour_level(bg)) = 101;
		io_pixel_colour_blue(io_colour_level(bg)) = 102;

		io_graphics_context_set_colour (gfx,bg);
		io_graphics_context_draw_pixel (gfx,pt);
		io_graphics_context_get_pixel (gfx,pt,&px);
		VERIFY (
				io_pixel_colour_red(px) == 100
			&&	io_pixel_colour_green(px) == 101
			&&	io_pixel_colour_blue(px) == 102,
			NULL
		);

		free_io_graphics_context(gfx);
	}

	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_graphics_text_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;
	io_graphics_context_t *gfx;

	uint8_t expect[] = {
		1,0,0,0,0,0,
		1,0,0,0,0,0,
		1,0,0,0,0,0,
		1,0,0,0,0,0,
		1,0,0,0,0,0,
		1,0,0,0,0,0,
		1,1,1,1,1,1,
	};
	io_byte_memory_get_info (bm,&bm_begin);

	gfx = mk_test_io_graphics_graphics_context(bm,IO_BLACK_8BIT);
	if (VERIFY (gfx != NULL,NULL)) {

		io_graphics_font_set_pixel_height (gfx->current_font,10);
		VERIFY (io_graphics_font_get_pixel_height (gfx->current_font) == 10,NULL);

		io_graphics_context_set_colour (gfx,IO_WHITE_8BIT);
		io_graphics_context_draw_character (gfx,'L',(io_i32_point_t){0,0});

		VERIFY (check_gfx_pattern (expect,6,7,gfx),NULL);

		free_io_graphics_context(gfx);
	}

	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_graphics_text_2) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;
	io_graphics_context_t *gfx;

	io_byte_memory_get_info (bm,&bm_begin);

	gfx = mk_test_io_graphics_graphics_context(bm,IO_BLACK_8BIT);
	if (VERIFY (gfx != NULL,NULL)) {
		uint8_t expect[] = {
			1,0,0,0,0,0,0,1,0,0,0,0,1,
			1,0,0,0,0,0,0,1,0,0,0,0,1,
			1,0,0,0,0,0,0,0,1,0,0,1,0,
			1,0,0,0,0,0,0,0,0,1,1,0,0,
			1,0,0,0,0,0,0,0,1,0,0,1,0,
			1,0,0,0,0,0,0,1,0,0,0,0,1,
			1,1,1,1,1,1,0,1,0,0,0,0,1,
		};

		io_graphics_font_set_pixel_height (gfx->current_font,10);
		VERIFY (io_graphics_font_get_pixel_height (gfx->current_font) == 10,NULL);

		io_graphics_context_set_colour (gfx,IO_WHITE_8BIT);
		
		io_graphics_context_begin (gfx);
		io_character_t text[] = {'L','X'};
		io_graphics_stack_append_text (
			io_graphics_context_get_command_stack(gfx),
			text,SIZEOF(text),
			def_i32_point(0,0)
		);
		io_graphics_context_run (gfx);

		VERIFY (check_gfx_pattern (expect,13,7,gfx),NULL);

		free_io_graphics_context(gfx);
	}

	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_graphics_line_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_graphics_context_t *gfx;

	gfx = mk_test_io_graphics_graphics_context(bm,IO_BLACK_8BIT);
	if (VERIFY (gfx != NULL,NULL)) {
		uint8_t expect[] = {
			1,1,0,0,0,0,
			0,0,0,0,0,0,
		};

		io_graphics_context_set_colour (gfx,IO_WHITE_8BIT);
		io_graphics_context_begin (gfx);
		io_graphics_stack_append_line (
			io_graphics_context_get_command_stack(gfx),
			def_i32_point(0,0),
			def_i32_point(1,0)
		);
		io_graphics_context_run (gfx);
		VERIFY (check_gfx_pattern (expect,6,2,gfx),NULL);

		free_io_graphics_context(gfx);
	}
}
TEST_END

TEST_BEGIN(test_io_graphics_circle_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_graphics_context_t *gfx;

	gfx = mk_test_io_graphics_graphics_context(bm,IO_WHITE_8BIT);
	if (VERIFY (gfx != NULL,NULL)) {
		uint8_t expect[] = {
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,1,1,1,0,0,0,
			0,0,1,1,0,0,0,1,1,0,
			0,0,1,0,0,0,0,0,1,0,
			0,1,0,0,0,0,0,0,0,1,
			0,1,0,0,0,0,0,0,0,1,
			0,1,0,0,0,0,0,0,0,1,
			0,0,1,0,0,0,0,0,1,0,
			0,0,1,1,0,0,0,1,1,0,
			0,0,0,0,1,1,1,0,0,0,
		};

		io_graphics_context_begin (gfx);
		io_graphics_stack_append_circle (
			io_graphics_context_get_command_stack(gfx),
			def_i32_point(5,5),4,false
		);
		io_graphics_context_run (gfx);

		VERIFY (check_gfx_pattern (expect,10,10,gfx),NULL);

		free_io_graphics_context(gfx);
	}
}
TEST_END

TEST_BEGIN(test_io_graphics_circle_2) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_graphics_context_t *gfx;

	gfx = mk_test_io_graphics_graphics_context(bm,IO_BLACK_8BIT);
	if (VERIFY (gfx != NULL,NULL)) {
		uint8_t expect[] = {
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,1,1,1,0,0,0,
			0,0,1,1,1,1,1,1,1,0,
			0,0,1,1,1,1,1,1,1,0,
			0,1,1,1,1,1,1,1,1,1,
			0,1,1,1,1,1,1,1,1,1,
			0,1,1,1,1,1,1,1,1,1,
			0,0,1,1,1,1,1,1,1,0,
			0,0,1,1,1,1,1,1,1,0,
			0,0,0,0,1,1,1,0,0,0,
		};

		io_graphics_context_set_colour (gfx,IO_WHITE_8BIT);
		io_graphics_context_begin (gfx);
		io_graphics_stack_append_circle (
			io_graphics_context_get_command_stack(gfx),
			def_i32_point(5,5),4,true
		);
		io_graphics_context_run (gfx);

		VERIFY (check_gfx_pattern (expect,10,10,gfx),NULL);

		free_io_graphics_context(gfx);
	}
}
TEST_END

TEST_BEGIN(test_io_graphics_rectangle_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_graphics_context_t *gfx;

	gfx = mk_test_io_graphics_graphics_context(bm,IO_BLACK_8BIT);
	if (VERIFY (gfx != NULL,NULL)) {
		uint8_t expect[] = {
			1,1,1,1,0,0,0,0,0,0,
			1,0,0,1,0,0,0,0,0,0,
			1,1,1,1,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
		};

		io_graphics_context_set_colour (gfx,IO_WHITE_8BIT);

		io_graphics_context_begin (gfx);
		io_graphics_stack_append_rectangle (
			io_graphics_context_get_command_stack(gfx),
			def_i32_point(0,0),def_i32_point(3,2),false
		);
		io_graphics_context_run (gfx);

		VERIFY (check_gfx_pattern (expect,10,10,gfx),NULL);

		free_io_graphics_context(gfx);
	}
}
TEST_END

TEST_BEGIN(test_io_graphics_rectangle_2) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_graphics_context_t *gfx;

	gfx = mk_test_io_graphics_graphics_context(bm,IO_BLACK_8BIT);
	if (VERIFY (gfx != NULL,NULL)) {
		uint8_t expect[] = {
			1,1,1,1,0,0,0,0,0,0,
			1,1,1,1,0,0,0,0,0,0,
			1,1,1,1,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
		};

		io_graphics_context_set_colour (gfx,IO_WHITE_8BIT);
		io_graphics_context_begin (gfx);
		io_graphics_stack_append_rectangle (
			io_graphics_context_get_command_stack(gfx),
			def_i32_point(0,0),def_i32_point(3,2),true
		);
		io_graphics_context_run (gfx);
		VERIFY (check_gfx_pattern (expect,10,10,gfx),NULL);

		free_io_graphics_context(gfx);
	}
}
TEST_END

UNIT_SETUP(setup_io_graphics_unit_test) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_byte_memory_get_info (bm,TEST_MEMORY_INFO);
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_graphics_unit_test) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_end;
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == TEST_MEMORY_INFO->used_bytes,NULL);
}

void
io_graphics_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_io_graphics_float_1,
		test_io_graphics_colour_1,
		test_io_graphics_1,
		test_io_graphics_text_1,
		test_io_graphics_text_2,
		test_io_graphics_line_1,
		test_io_graphics_circle_1,
		test_io_graphics_circle_2,
		test_io_graphics_rectangle_1,
		test_io_graphics_rectangle_2,
		0
	};
	unit->name = "io graphics";
	unit->description = "io graphics unit test";
	unit->tests = tests;
	unit->setup = setup_io_graphics_unit_test;
	unit->teardown = teardown_io_graphics_unit_test;
}
#ifdef IMPLEMENT_VERIFY_IO_CORE_GRAPHICS_FONT_ROBOTO
TEST_BEGIN(test_io_graphics_font_roboto_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_graphics_context_t *gfx;

	gfx = mk_test_io_graphics_graphics_context(bm,IO_BLACK_8BIT);
	io_graphics_font_set_pixel_height (gfx->current_font,40);
	io_graphics_context_set_colour (gfx,IO_WHITE_8BIT);

	io_graphics_context_select_font_by_name (gfx,(uint8_t const*) "RobotoBold",10);
	io_graphics_context_draw_ascii_text (gfx,"L4g",(io_i32_point_t){0,-9});
	//io_graphics_context_render(gfx);

	free_io_graphics_context(gfx);
}

TEST_END
UNIT_SETUP(setup_io_graphics_font_roboto_unit_test) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_byte_memory_get_info (bm,TEST_MEMORY_INFO);
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_graphics_font_robotos_unit_test) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_end;
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == TEST_MEMORY_INFO->used_bytes,NULL);
}

void
io_graphics_font_roboto_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_io_graphics_font_roboto_1,
		0
	};
	unit->name = "roboto font";
	unit->description = "io graphics roboto font unit test";
	unit->tests = tests;
	unit->setup = setup_io_graphics_font_roboto_unit_test;
	unit->teardown = teardown_io_graphics_font_robotos_unit_test;
}
# define IO_CORE_GRAPHICS_FONT_ROBOTO_UT \
io_graphics_font_roboto_unit_test,\
	/**/
#else
# define IO_CORE_GRAPHICS_FONT_ROBOTO_UT
#endif /* IMPLEMENT_VERIFY_IO_CORE_GRAPHICS_FONT_ROBOTO */

void
run_ut_io_core_graphics (V_runner_t *runner) {
	static const unit_test_t test_set[] = {
		io_graphics_commands_unit_test,
		io_graphics_unit_test,
		IO_CORE_GRAPHICS_FONT_ROBOTO_UT
		0
	};
	V_run_unit_tests(runner,test_set);
}
#endif /* IMPLEMENT_VERIFY_IO_CORE_GRAPHICS */

#endif
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
