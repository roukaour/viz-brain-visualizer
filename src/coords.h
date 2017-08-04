#ifndef COORDS_H
#define COORDS_H

// Model-related coordinates (used for soma locations, synapse locations, and field bounds)
// can be stored as different data types depending on the application's needs. Short integers
// take up less space, but sometimes coordinates exceed their range. Integers (short or not)
// are faster when using a CPU's integrated graphics processor, but dedicated graphics cards
// are fastest with floating-point coordinates.

#include "utils.h"

// Two-byte "short" integer coordinates can range from -32,768 to 32,767
// (not sufficient for all models)
#if defined(SHORT_COORDS)
typedef int16_t coord_t;
#define ZERO_COORD 0
#define UNIT_COORD 1
#define MIN_COORD ((std::numeric_limits<coord_t>::min)())
#define MAX_COORD ((std::numeric_limits<coord_t>::max)())
#define glVertex3c glVertex3s
#define glVertex3cv glVertex3sv
#define glRasterPos3c glRasterPos3s
#define glRasterPos3cv glRasterPos3sv
// Four-byte integer coordinates can range from -2^31 to 2^31-1
#elif defined(INT_COORDS)
typedef int32_t coord_t;
#define ZERO_COORD 0
#define UNIT_COORD 1
#define MIN_COORD ((std::numeric_limits<coord_t>::min)())
#define MAX_COORD ((std::numeric_limits<coord_t>::max)())
#define glVertex3c glVertex3i
#define glVertex3cv glVertex3iv
#define glRasterPos3c glRasterPos3i
#define glRasterPos3cv glRasterPos3iv
#else
// Four-byte floating-point coordinates can range from around -3.4*10^38 to 3.4*10^38
typedef float coord_t;
#define ZERO_COORD 0.0f
#define UNIT_COORD 1.0f
#define MIN_COORD (-(std::numeric_limits<coord_t>::max)())
#define MAX_COORD ((std::numeric_limits<coord_t>::max)())
#define glVertex3c glVertex3f
#define glVertex3cv glVertex3fv
#define glRasterPos3c glRasterPos3f
#define glRasterPos3cv glRasterPos3fv
#endif

#endif
