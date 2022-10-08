// Midnight OS --
#if !defined(_cplusplus)
	#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
	#error "You're doing wrong. You need to use cross-compiler."
#endif

#if !defined(__i386__)
	#error "You're doing wrong. You need to use ix86-elf compiler."
#endif

/* fake VGA driver */
enum vga_color {
	COLOR_BLACK = 0,
	COLOR_BLUE = 1,
	COLOR_GREEN = 2,
	COLOR_CYAN = 3,
	COLOR_RED = 4,
	COLOR_MAGENTA = 5,
	COLOR_BROWN = 6,
	COLOR_LIGHT_GREY = 7,
	COLOR_DARK_GREY = 8,
	COLOR_LIGHT_BLUE = 9,
	COLOR_LIGHT_GREEN = 10,
	COLOR_LIGHT_CYAN = 11,
	COLOR_LIGHT_RED = 12,
	COLOR_LIGHT_MAGENTA = 13,
	COLOR_LIGHT_BROWN = 14,
	COLOR_WHITE = 15
};

uint8_t make_color(enum vga_color fg, enum vga_color bg)
{
	return fg | bg << 4;
}

uint16_t make_vgaentry(char c, uint8_t color)
{
	uint16_t c16 = c;
	uint16_t color16 = color;
	return c16 | color16 << 8;
}

/* strlen */
size_t strlen(const char* str)
{
	size_t ret = 0;
	
	while(str[ret] != 0)
		ret++;

	return ret;
}

/* terminal */
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void terminal_cleanup()
{
	for(size_t y = 0; y < VGA_HEIGHT; y++)
	{
		for(size_t x = 0; x < VGA_WIDTH; x++)
		{
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = make_vgaentry(' ', terminal_color);
		}
	}
}

void terminal_initialize()
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = make_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
	terminal_buffer = (uint16_t*) 0xB8000;
	terminal_cleanup();
}

void terminal_setcolor(uint8_t color)
{
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = make_vgaentry(c, color);
}

void terminal_putchar(char c)
{
	if(c == '\n')
	{
		terminal_row++;
		terminal_column = 1;
	}
	else
	{
		terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	}

	if(++terminal_column == VGA_WIDTH)
	{
		terminal_row++;
		terminal_column = 0;
	}

	if(terminal_row == VGA_HEIGHT)
	{
		terminal_row = VGA_HEIGHT - 1;
		terminal_column = 2;

		for(size_t y = 1; y < VGA_HEIGHT; y++)
		{
			for(size_t x = 0; x < VGA_WIDTH; x++)
			{
				const size_t index = y * VGA_WIDTH + x;
				terminal_buffer[index - VGA_WIDTH] = terminal_buffer[index];

				if(y == VGA_HEIGHT - 1)
				{
					terminal_putentryat(' ', terminal_color, x, y);
				}
			}
		}
	}
}

void terminal_writestring(const char* data)
{
	size_t datalen = strlen(data);

	for(size_t i = 0; i < datalen; i++)
	{
		terminal_putchar(data[i]);
	}
}

/* Input/output */
#define CHAR_NULL	0
#define CHAR_UNK	0
#define CHAR_REL	9619691
#define	CHAR_ESC	27
#define CHAR_CTRL	0
#define CHAR_SHIFT	0

unsigned char scanCodes[128] = {
	CHAR_NULL, CHAR_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
	CHAR_CTRL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', ' ', CHAR_SHIFT, '\\',
	'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', CHAR_NULL, CHAR_NULL, CHAR_NULL, ' '
};

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

size_t get_scancode()
{
	size_t c = 0;

	do {
		if(inb(0x60) != c)
		{
			c = inb(0x60);

			if((c & 128) == 128)
			{
				return CHAR_REL;
			}
			else
			{
				return c;
			}
		}
	} while(1);
}

char get_character()
{
	return scanCodes[get_scancode()];
}

/* os */
#if defined(__cplusplus)
	extern "C"
#endif

void kernel_main()
{
	terminal_initialize();
	terminal_writestring("Welcome to Midnight..\n");
	terminal_setcolor(make_color(COLOR_LIGHT_GREY, COLOR_BLACK));
	terminal_writestring("Command line is now alive. Get help with / command.\n");

	bool looping = true;
	bool updatelock = false;
	char lastentry = CHAR_NULL;
	char lastinput = CHAR_NULL;

	while(looping)
	{
		if(get_scancode() == CHAR_REL)
		{
			updatelock = false;
		}

		lastinput = get_character();
		
		if(updatelock == false && lastinput != CHAR_NULL)
		{
			updatelock = true;

			switch(lastinput)
			{
				case '\b':
					terminal_putentryat(' ', terminal_color, terminal_column - 1, terminal_row);
					terminal_column = terminal_column - 1;
					break;

				case '\n':
					if(lastentry == '/')
					{
						terminal_writestring("\nA very early stage. This is for test.");
					}

					terminal_row++;
					terminal_column = 1;
					lastentry = CHAR_NULL;
					break;

				case CHAR_ESC:
					looping = false;
					break;

				default:
					terminal_putchar(lastinput);
					lastentry = lastinput;
					break;
			}
		}
	}

	terminal_writestring("\nExit music. . .\nMidnight exited its loop, you should reboot or shutdown manually.");
}