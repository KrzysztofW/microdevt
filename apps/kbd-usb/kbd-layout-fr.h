#include "lufa-key-codes.h"

static key_map_t kbd_layout_fr[] = {
	/* char, modfier, lufa code */
	{'`', 0xe6, 0x24},
	{'1', 0xe5, 0x1e},
	{'2', 0xe5, 0x1f},
	{'3', 0xe5, 0x20},
	{'4', 0xe5, 0x21},
	{'5', 0xe5, 0x22},
	{'6', 0xe5, 0x23},
	{'7', 0xe5, 0x24},
	{'8', 0xe5, 0x25},
	{'9', 0xe5, 0x26},
	{'0', 0xe5, 0x27},
	{'-', 0x0, 0x23},
	{'=', 0x0, 0x2e},
	{'q', 0x0, 0x4},
	{'w', 0x0, 0x1d},
	{'e', 0x0, 0x8},
	{'r', 0x0, 0x15},
	{'t', 0x0, 0x17},
	{'y', 0x0, 0x1c},
	{'u', 0x0, 0x18},
	{'i', 0x0, 0xc},
	{'o', 0x0, 0x12},
	{'p', 0x0, 0x13},
	{'[', 0xe6, 0x22},
	{']', 0xe6, 0x2d},
	{'a', 0x0, 0x14},
	{'s', 0x0, 0x16},
	{'d', 0x0, 0x7},
	{'f', 0x0, 0x9},
	{'g', 0x0, 0xa},
	{'h', 0x0, 0xb},
	{'j', 0x0, 0xd},
	{'k', 0x0, 0xe},
	{'l', 0x0, 0xf},
	{';', 0x0, 0x36},
	{'\'', 0x0, 0x21},
	{'\\', 0xe6, 0x25},
	{'z', 0x0, 0x1a},
	{'x', 0x0, 0x1b},
	{'c', 0x0, 0x6},
	{'v', 0x0, 0x19},
	{'b', 0x0, 0x5},
	{'n', 0x0, 0x11},
	{'m', 0x0, 0x33},
	{',', 0x0, 0x10},
	{'.', 0xe5, 0x36},
	{'/', 0xe5, 0x37},
	{'~', 0xe6, 0x1f},
	{'!', 0x0, 0x38},
	{'@', 0xe6, 0x27},
	{'#', 0xe6, 0x20},
	{'$', 0x0, 0x30},
	{'%', 0xe5, 0x34},
	{'^', 0xe6, 0x26},
	{'&', 0x0, 0x1e},
	{'*', 0x0, 0x31},
	{'(', 0x0, 0x22},
	{')', 0x0, 0x2d},
	{'_', 0x0, 0x25},
	{'+', 0xe5, 0x2e},
	{'Q', 0xe1, 0x4},
	{'W', 0xe1, 0x1d},
	{'E', 0xe1, 0x8},
	{'R', 0xe1, 0x15},
	{'T', 0xe1, 0x17},
	{'Y', 0xe1, 0x1c},
	{'U', 0xe1, 0x18},
	{'I', 0xe1, 0xc},
	{'O', 0xe1, 0x12},
	{'P', 0xe1, 0x13},
	{'{', 0xe6, 0x21},
	{'}', 0xe6, 0x2e},
	{'A', 0xe1, 0x14},
	{'S', 0xe1, 0x16},
	{'D', 0xe1, 0x7},
	{'F', 0xe1, 0x9},
	{'G', 0xe1, 0xa},
	{'H', 0xe1, 0xb},
	{'J', 0xe1, 0xd},
	{'K', 0xe1, 0xe},
	{'L', 0xe1, 0xf},
	{':', 0x0, 0x37},
	{'"', 0x0, 0x20},
	{'|', 0xe6, 0x23},
	{'Z', 0xe1, 0x1a},
	{'X', 0xe1, 0x1b},
	{'C', 0xe1, 0x6},
	{'V', 0xe1, 0x19},
	{'B', 0xe1, 0x5},
	{'N', 0xe1, 0x11},
	{'M', 0xe1, 0x33},
	{'<', 0x0, 0x36},
	{'>', 0xe5, 0x36},
	{'?', 0xe5, 0x10},
	{'\n', 0, HID_KEYBOARD_SC_ENTER},
	{'\t', 0, HID_KEYBOARD_SC_TAB},
	{' ', 0, HID_KEYBOARD_SC_SPACE},
};
