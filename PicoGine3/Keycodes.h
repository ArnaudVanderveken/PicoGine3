#ifndef KEYCODES_H
#define KEYCODES_H

enum KeyCode : BYTE
{
	KC_MOUSE1 = 0x01,
	KC_MOUSE2 = 0x02,
	KC_CANCEL = 0x03,
	KC_MOUSE3 = 0x04,
	KC_MOUSE4 = 0x05,
	KC_MOUSE5 = 0x06,
	KC_BACKSPACE = 0x08,
	KC_TAB = 0x09,
	KC_CLEAR = 0x0C,
	KC_ENTER = 0x0D,
	KC_SHIFT = 0x10,
	KC_CONTROL = 0x11,
	KC_ALT = 0x12,
	KC_PAUSE = 0x13,
	KC_CAPS_LOCK = 0x14,
	KC_ESCAPE = 0x1B,
	KC_SPACEBAR = 0x20,
	KC_PAGE_UP = 0x21,
	KC_PAGE_DOWN = 0x22,
	KC_END = 0x23,
	KC_HOME = 0x24,
	KC_LEFT = 0x25,
	KC_UP = 0x26,
	KC_RIGHT = 0x27,
	KC_DOWN = 0x28,
	KC_SELECT = 0x29,
	KC_PRINT = 0x2A,
	KC_EXECUTE = 0x2B,
	KC_PRINT_SCREEN = 0x2C,
	KC_INSERT = 0x2D,
	KC_DELETE = 0x2E,
	KC_HELP = 0x2F,
	KC_0 = 0x30,
	KC_1 = 0x31,
	KC_2 = 0x32,
	KC_3 = 0x33,
	KC_4 = 0x34,
	KC_5 = 0x35,
	KC_6 = 0x36,
	KC_7 = 0x37,
	KC_8 = 0x38,
	KC_9 = 0x39,
	KC_A = 0x41,
	KC_B = 0x42,
	KC_C = 0x43,
	KC_D = 0x44,
	KC_E = 0x45,
	KC_F = 0x46,
	KC_G = 0x47,
	KC_H = 0x48,
	KC_I = 0x49,
	KC_J = 0x4A,
	KC_K = 0x4B,
	KC_L = 0x4C,
	KC_M = 0x4D,
	KC_N = 0x4E,
	KC_O = 0x4F,
	KC_P = 0x50,
	KC_Q = 0x51,
	KC_R = 0x52,
	KC_S = 0x53,
	KC_T = 0x54,
	KC_U = 0x55,
	KC_V = 0x56,
	KC_W = 0x57,
	KC_X = 0x58,
	KC_Y = 0x59,
	KC_Z = 0x5A,
	KC_L_WIN = 0x5B,
	KC_R_WIN = 0x5C,
	KC_APPLICATIONS = 0x5D,
	KC_NUMPAD_0 = 0x60,
	KC_NUMPAD_1 = 0x61,
	KC_NUMPAD_2 = 0x62,
	KC_NUMPAD_3 = 0x63,
	KC_NUMPAD_4 = 0x64,
	KC_NUMPAD_5 = 0x65,
	KC_NUMPAD_6 = 0x66,
	KC_NUMPAD_7 = 0x67,
	KC_NUMPAD_8 = 0x68,
	KC_NUMPAD_9 = 0x69,
	KC_MULTIPLY = 0x6A,
	KC_ADD = 0x6B,
	KC_SEPARATOR = 0x6C,
	KC_SUBTRACT = 0x6D,
	KC_DECIMAL = 0x6E,
	KC_DIVIDE = 0x6F,
	KC_F1 = 0x70,
	KC_F2 = 0x71,
	KC_F3 = 0x72,
	KC_F4 = 0x73,
	KC_F5 = 0x74,
	KC_F6 = 0x75,
	KC_F7 = 0x76,
	KC_F8 = 0x77,
	KC_F9 = 0x78,
	KC_F10 = 0x79,
	KC_F11 = 0x7A,
	KC_F12 = 0x7B,
	KC_F13 = 0x7C,
	KC_F14 = 0x7D,
	KC_F15 = 0x7E,
	KC_F16 = 0x7F,
	KC_F17 = 0x80,
	KC_F18 = 0x81,
	KC_F19 = 0x82,
	KC_F20 = 0x83,
	KC_F21 = 0x84,
	KC_F22 = 0x85,
	KC_F23 = 0x86,
	KC_F24 = 0x87,
	KC_NUM_LOCK = 0x90,
	KC_SCROLL_LOCK = 0x91,
	KC_L_SHIFT = 0xA0,
	KC_R_SHIFT = 0xA1,
	KC_L_CONTROL = 0xA2,
	KC_R_CONTROL = 0xA3,
	KC_L_ALT = 0xA4,
	KC_R_ALT = 0xA5,
	KC_SEMICOLON = 0xBA,
	KC_PLUS = 0xBB,
	KC_COMA = 0xBC,
	KC_MINUS = 0xBD,
	KC_PERIOD = 0xBE,
	KC_SLASH = 0xBF,
	KC_BACKTICK = 0xC0,
	KC_BRACKETS_OPEN = 0xDB,
	KC_BACKSLASH = 0xDC,
	KC_BRACKETS_CLOSE = 0xDD,
	KC_QUOTE = 0xDE,
};

enum ControllerCode
{
	GP_DPAD_UP = 0,
	GP_DPAD_DOWN = 1,
	GP_DPAD_LEFT = 2,
	GP_DPAD_RIGHT = 3,
	GP_START = 4,
	GP_BACK = 5,
	GP_LEFT_THUMB = 6,
	GP_RIGHT_THUMB = 7,
	GP_LEFT_SHOULDER = 8,
	GP_RIGHT_SHOULDER = 9,
	GP_A = 12,
	GP_B = 13,
	GP_X = 14,
	GP_Y = 15,
	GP_COUNT = 16
};

#endif //KEYCODES_H