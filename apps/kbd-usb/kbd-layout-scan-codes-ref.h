#ifndef __KBD_LAYOUT_SCAN_CODES_US_H__
#define __KBD_LAYOUT_SCAN_CODES_US_H__
#include "lufa-key-codes.h"

// `x1234567890-=qwertyuiop[]asdfghjkl;'\<zxcvbnm,./

static key_map_t kbd_layout_scan_codes[] = {
	/* char, scan code, lufa code*/
	 {'`', 0x29, 53},
	 {'1', 0x02, 30},
	 {'2', 0x03, 31},
	 {'3', 0x04, 32},
	 {'4', 0x05, 33},
	 {'5', 0x06, 34},
	 {'6', 0x07, 35},
	 {'7', 0x08, 36},
	 {'8', 0x09, 37},
	 {'9', 0x0A, 38},
	 {'0', 0x0B, 39},
	 {'-', 0x0C, 45},
	 {'=', 0x0D, 46},
	 {'q', 0x10, 20},
	 {'w', 0x11, 26},
	 {'e', 0x12, 8},
	 {'r', 0x13, 21},
	 {'t', 0x14, 23},
	 {'y', 0x15, 28},
	 {'u', 0x16, 24},
	 {'i', 0x17, 12},
	 {'o', 0x18, 18},
	 {'p', 0x19, 19},
	 {'[', 0x1A, 47},
	 {']', 0x1B, 48},
	 {'a', 0x1E, 4},
	 {'s', 0x1F, 22},
	 {'d', 0x20, 7},
	 {'f', 0x21, 9},
	 {'g', 0x22, 10},
	 {'h', 0x23, 11},
	 {'j', 0x24, 13},
	 {'k', 0x25, 14},
	 {'l', 0x26, 15},
	 {';', 0x27, 51},
	 {'\'', 0x28, 52},
	 {'\\', 0x2B, HID_KEYBOARD_SC_BACKSLASH_AND_PIPE},
	 {'<', 0x56, HID_KEYBOARD_SC_COMMA_AND_LESS_THAN_SIGN},
	 {'z', 0x2C, 29},
	 {'x', 0x2D, 27},
	 {'c', 0x2E, 6},
	 {'v', 0x2F, 25},
	 {'b', 0x30, 5},
	 {'n', 0x31, 17},
	 {'m', 0x32, 16},
	 {',', 0x33, 54},
	 {'.', 0x34, 55},
	 {'/', 0x35, 56},
	 /* these are always constant on all keyboards */
	 {'\n', 0x1C, HID_KEYBOARD_SC_ENTER},
	 {'\t', 0x0F, HID_KEYBOARD_SC_TAB},
	 {' ', 0x39, HID_KEYBOARD_SC_SPACE},
};

#endif
