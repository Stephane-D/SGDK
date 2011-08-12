#include "genesis.h"

//    CUBE
//
//    7----6
//   /|   /|
//  3-+--2 |
//  | |  | |
//  | 4--+-5
//  |/   |/
//  0----1

const Vect3D_f16 cube_coord[8] =
{
    {intToFix16(-1), intToFix16(-1), intToFix16(-1)},
    {intToFix16(1), intToFix16(-1), intToFix16(-1)},
    {intToFix16(1), intToFix16(1), intToFix16(-1)},
    {intToFix16(-1), intToFix16(1), intToFix16(-1)},
    {intToFix16(-1), intToFix16(-1), intToFix16(1)},
    {intToFix16(1), intToFix16(-1), intToFix16(1)},
    {intToFix16(1), intToFix16(1), intToFix16(1)},
    {intToFix16(-1), intToFix16(1), intToFix16(1)}
};


const u16 cube_poly_ind[6 * 4] =
{
    7, 4, 5, 6,
    0, 3, 2, 1,
    3, 7, 6, 2,
    4, 0, 1, 5,
    1, 2, 6, 5,
    4, 7, 3, 0
};

const u16 cube_line_ind[12 * 2] =
{
    0, 1,
    1, 2,
    2, 3,
    3, 0,
    4, 5,
    5, 6,
    6, 7,
    7, 4,
    0, 4,
    1, 5,
    2, 6,
    3, 7
};

const Vect3D_f16 cube_face_norm[6] =
{
    {intToFix16(0), intToFix16(0), intToFix16(1)},
    {intToFix16(0), intToFix16(0), intToFix16(-1)},
    {intToFix16(0), intToFix16(1), intToFix16(0)},
    {intToFix16(0), intToFix16(-1), intToFix16(0)},
    {intToFix16(1), intToFix16(0), intToFix16(0)},
    {intToFix16(-1), intToFix16(0), intToFix16(0)}
};

const Vect3D_f16 cube_pts_norm[8] =
{
    {FIX16(-0.57735), FIX16(-0.57735), FIX16(-0.57735)},
    {FIX16(0.57735), FIX16(-0.57735), FIX16(-0.57735)},
    {FIX16(0.57735), FIX16(0.57735), FIX16(-0.57735)},
    {FIX16(-0.57735), FIX16(0.57735), FIX16(-0.57735)},
    {FIX16(-0.57735), FIX16(-0.57735), FIX16(0.57735)},
    {FIX16(0.57735), FIX16(-0.57735), FIX16(0.57735)},
    {FIX16(0.57735), FIX16(0.57735), FIX16(0.57735)},
    {FIX16(-0.57735), FIX16(0.57735), FIX16(0.57735)}
};
