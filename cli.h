#ifndef __CLI_H__
#define __CLI_H__

#include <conio.h>
#include <i86.h>

#if (defined(_DOS) || defined(__DOS__))

unsigned long pushf();
#pragma aux pushf = "pushfd" "pop eax" value [eax] modify [eax]

void popf(unsigned long);
#pragma aux popf = "push eax" "popfd" parm [eax] modify [eax]

inline void _enable_if_enabled(unsigned long flags) { if (flags & 0x200) _disable(); }

#else 

// multitasking systems will kick you for these tricks    

#error "pushf/popf are risky on non-DOS environments, so you are warned"

#define pushf(a)              {};
#define popf(a)               {};
#define _enable_if_enabled(a) {};

#endif

#endif