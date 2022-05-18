#ifndef CSKEYS_H
#define CSKEYS_H
typedef enum _ELWJGLKey {
	LWKEY_ESCAPE = 1,
	LWKEY_1,        
	LWKEY_2,        
	LWKEY_3,        
	LWKEY_4,        
	LWKEY_5,        
	LWKEY_6,        
	LWKEY_7,        
	LWKEY_8,        
	LWKEY_9,        
	LWKEY_0,        
	LWKEY_MINUS,    
	LWKEY_EQUALS,   
	LWKEY_BACKSPACE,
	LWKEY_TAB,      
	LWKEY_Q,
	LWKEY_W,
	LWKEY_E,
	LWKEY_R,
	LWKEY_T,
	LWKEY_Y,
	LWKEY_U,
	LWKEY_I,
	LWKEY_O,
	LWKEY_P,
	LWKEY_LBRACKET,
	LWKEY_RBRACKET,
	LWKEY_ENTER,
	LWKEY_LCTRL,
	LWKEY_A,
	LWKEY_S,
	LWKEY_D,
	LWKEY_F,
	LWKEY_G,
	LWKEY_H,
	LWKEY_J,
	LWKEY_K,
	LWKEY_L,
	LWKEY_SEMICOLON,
	LWKEY_QUOTE,
	LWKEY_TILDE,
	LWKEY_LSHIFT,
	LWKEY_BACKSLASH,
	LWKEY_Z,
	LWKEY_X,
	LWKEY_C,
	LWKEY_V,
	LWKEY_B,
	LWKEY_N,
	LWKEY_M,
	LWKEY_COMMA,
	LWKEY_PERIOD,
	LWKEY_SLASH,
	LWKEY_RSHIFT,
	LWKEY_LALT = 56,
	LWKEY_SPACE,
	LWKEY_CAPSLOCK,
	LWKEY_F1,
	LWKEY_F2,
	LWKEY_F3,
	LWKEY_F4,
	LWKEY_F5,
	LWKEY_F6,
	LWKEY_F7,
	LWKEY_F8,
	LWKEY_F9,
	LWKEY_F10,
	LWKEY_NUMLOCK,
	LWKEY_SCROLLLOCK,
	LWKEY_KP7,
	LWKEY_KP8,
	LWKEY_KP9,
	LWKEY_KP_MINUS,
	LWKEY_KP4,
	LWKEY_KP5,
	LWKEY_KP6,
	LWKEY_KP_PLUS,
	LWKEY_KP1,
	LWKEY_KP2,
	LWKEY_KP3,
	LWKEY_KP0,
	LWKEY_KP_DECIMAL,
	LWKEY_F11 = 87,
	LWKEY_F12,
	LWKEY_F13 = 100,
	LWKEY_F14,
	LWKEY_F15,
	LWKEY_F16,
	LWKEY_F17,
	LWKEY_F18,
	LWKEY_KP_EQU = 141,
	LWKEY_KP_ENTER = 156,
	LWKEY_RCTRL,
	LWKEY_KP_DIVIDE = 181,
	LWKEY_RALT = 184,
	LWKEY_PAUSE = 197,
	LWKEY_HOME = 199,
	LWKEY_UP,
	LWKEY_PAGEUP,
	LWKEY_LEFT = 203,
	LWKEY_RIGHT = 205,
	LWKEY_END = 207,
	LWKEY_DOWN,
	LWKEY_PAGEDOWN,
	LWKEY_INSERT,
	LWKEY_DELETE,
	LWKEY_LWIN = 219,
	LWKEY_RWIN
} ELWJGLKey;

typedef enum _ELWJGLMod {
	LWMOD_NONE = 0,
	LWMOD_CTRL = 1,
	LWMOD_SHIFT = 2,
	LWMOD_ALT = 4
} ELWJGLMod;
#endif