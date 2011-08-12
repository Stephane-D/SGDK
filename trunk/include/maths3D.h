#ifndef _MATHS3D_H_
#define _MATHS3D_H_


typedef struct
{
	Vect3D_f16 a;
	Vect3D_f16 b;
	Vect3D_f16 c;
} Mat3D_f16;


typedef struct
{
	fix16 Tx;
	fix16 Ty;
	fix16 Tz;
	fix16 Rx;
	fix16 Ry;
	fix16 Rz;
} Trans3D_f16;


extern Vect3D_f16 light_trans;
//extern Vect3D_f16 camview_trans;


void M3D_reset();

void M3D_setLightEnabled(u16 enabled);
u16  M3D_getLightEnabled();

void M3D_setViewport(u16 w, u16 h);
void M3D_setCamDist3D(fix16 value);
void M3D_setLightXYZ3D(fix16 x, fix16 y, fix16 z);
void M3D_setLight3D(Vect3D_f16 *value);

void M3D_resetMat3D();

void M3D_setTXMat3D(fix16 tx);
void M3D_setTYMat3D(fix16 ty);
void M3D_setTZMat3D(fix16 tz);
void M3D_setTXYZMat3D(fix16 tx, fix16 ty, fix16 tz);
void M3D_setTransMat3D(Vect3D_f16 *trans);

void M3D_setRXMat3D(fix16 rx);
void M3D_setRYMat3D(fix16 ry);
void M3D_setRZMat3D(fix16 rz);
void M3D_setRXYZMat3D(fix16 rx, fix16 ry, fix16 rz);
void M3D_setRotMat3D(Vect3D_f16 *rot);

void M3D_transform3D(const Vect3D_f16 *src, Vect3D_f16 *dest, u16 numv);
void M3D_project3D_f16_old(const Vect3D_f16 *src, Vect2D_f16 *dest, u16 numv);
void M3D_project3D_s16_old(const Vect3D_f16 *src, Vect2D_s16 *dest, u16 numv);


#endif // _MATHS3D_H_
