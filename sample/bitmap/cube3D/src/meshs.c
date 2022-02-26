#include "genesis.h"


//    CUBE
//
//    6----7
//   /|   /|
//  2-+--4 |
//  | |  | |
//  | 3--+-5
//  |/   |/
//  0----1

const Vect3D_f16 cube_coord[8] =
{
    {FIX16(-5), FIX16(-5), FIX16(-5)},
    {FIX16(5), FIX16(-5), FIX16(-5)},
    {FIX16(-5), FIX16(5), FIX16(-5)},
    {FIX16(-5), FIX16(-5), FIX16(5)},
    {FIX16(5), FIX16(5), FIX16(-5)},
    {FIX16(5), FIX16(-5), FIX16(5)},
    {FIX16(-5), FIX16(5), FIX16(5)},
    {FIX16(5), FIX16(5), FIX16(5)}
};

const u16 cube_poly_ind[6 * 4] =
{
    6, 3, 5, 7,
    0, 2, 4, 1,
    2, 6, 7, 4,
    3, 0, 1, 5,
    1, 4, 7, 5,
    3, 6, 2, 0
};

const u16 cube_line_ind[12 * 2] =
{
    0, 1,
    1, 4,
    4, 2,
    2, 0,
    3, 5,
    5, 7,
    7, 6,
    6, 3,
    0, 3,
    1, 5,
    4, 7,
    2, 6
};

const Vect3D_f16 cube_face_norm[6] =
{
    {FIX16(0), FIX16(0), FIX16(1)},
    {FIX16(0), FIX16(0), FIX16(-1)},
    {FIX16(0), FIX16(1), FIX16(0)},
    {FIX16(0), FIX16(-1), FIX16(0)},
    {FIX16(1), FIX16(0), FIX16(0)},
    {FIX16(-1), FIX16(0), FIX16(0)}
};
