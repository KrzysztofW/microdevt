#ifndef _MODIFIERS_H_
#define _MODIFIERS_H_

#define MODIFIER_ALT          56
#define MODIFIER_ALT_GT      100
#define MODIFIER_CTL_LEFT     97
#define MODIFIER_CTL_RIGHT    29
#define MODIFIER_SHIFT_RIGHT  54
#define MODIFIER_SHIFT_LEFT   42
#define MODIFIER_WIN         125

static inline int is_modifier(char c)
{
	switch (c) {
	case MODIFIER_WIN:
	case MODIFIER_ALT:
	case MODIFIER_ALT_GT:
	case MODIFIER_CTL_LEFT:
	case MODIFIER_CTL_RIGHT:
	case MODIFIER_SHIFT_LEFT:
	case MODIFIER_SHIFT_RIGHT:
		return 1;
	default:
		return 0;
	}
}

#endif
