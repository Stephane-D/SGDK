#ifndef _MATHS3D_H_
#define _MATHS3D_H_


#define MAT3D_MAXPOINTS  256
#define MAT3D_MAXFACE    256


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


extern Vect3D_f16 pts_3D[MAT3D_MAXPOINTS];
extern Vect2D_s16 pts_2D[MAT3D_MAXPOINTS];
//extern u16 face_index[MAT3D_MAXFACE];

extern Vect3D_f16 light_trans;
extern Vect3D_f16 camview_trans;


void reset3D();

void setCamDist3D(fix16 value);
void setLight3D(Vect3D_f16 *value);

void resetMat3D();

void setTXMat3D(fix16 tx);
void setTYMat3D(fix16 ty);
void setTZMat3D(fix16 tz);
void setTXYZMat3D(fix16 tx, fix16 ty, fix16 tz);
void setTransMat3D(Vect3D_f16 *trans);

void setRXMat3D(fix16 rx);
void setRYMat3D(fix16 ry);
void setRZMat3D(fix16 rz);
void setRXYZMat3D(fix16 rx, fix16 ry, fix16 rz);
void setRotMat3D(Vect3D_f16 *rot);

void transform3D(const Vect3D_f16 *src, Vect3D_f16 *dest, u16 numv);
void project3D_f16(const Vect3D_f16 *src, Vect2D_f16 *dest, u16 numv, u16 w, u16 h);
void project3D_s16(const Vect3D_f16 *src, Vect2D_s16 *dest, u16 numv, u16 w, u16 h);


#endif // _MATHS3D_H_
