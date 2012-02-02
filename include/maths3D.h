/**
 * \file maths3D.h
 * \brief 3D math engine.
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides 3D transformation methods :<br>
 * - translation X, Y, Z<br>
 * - rotation X, Y, Z<br>
 * - one directionnal light<br>
 * - 2D projection<br>
 *<br>
 * Can transform (including 2D projection) about ~7000 vertices / seconde.
 */

#ifndef _MATHS3D_H_
#define _MATHS3D_H_


/**
 *  \struct Mat3D_f16
 *      3x3 Matrice structure - f16 (fix16) type.<br>
 *      Internally uses 3 3D vectors.
 */
typedef struct
{
	Vect3D_f16 a;
	Vect3D_f16 b;
	Vect3D_f16 c;
} Mat3D_f16;


/**
 *  \brief
 *      Transformed Light vector (can be used for light normal calculation).
 */
extern Vect3D_f16 light_trans;
//extern Vect3D_f16 camview_trans;


/**
 *  \brief
 *      Reset math 3D engine (reset matrices and transformation parameters mainly).
 */
void M3D_reset();

/**
 *  \brief
 *      Enable or disable light transformation calculation.
 */
void M3D_setLightEnabled(u16 enabled);
/**
 *  \brief
 *      Get light transformation calculation enabled flag.
 */
u16  M3D_getLightEnabled();

/**
 *  \brief
 *      Set viewport dimension.
 *
 *  \param w
 *      Viewport width (use BMP_WIDTH if you use 3D with software bitmap engine)
 *  \param h
 *      Viewport height (use BMP_HEIGHT if you use 3D with software bitmap engine)
 */
void M3D_setViewport(u16 w, u16 h);
/**
 *  \brief
 *      Set camera scene distance.
 *
 *  \param value
 *      Distance between the camera and the scene.
 */
void M3D_setCamDist3D(fix16 value);
/**
 *  \brief
 *      Set light direction vector.
 */
void M3D_setLightXYZ3D(fix16 x, fix16 y, fix16 z);
/**
 *  \brief
 *      Set light direction vector.
 */
void M3D_setLight3D(Vect3D_f16 *value);

/**
 *  \brief
 *      Reset the transformation matrice.
 */
void M3D_resetMat3D();

/**
 *  \brief
 *      Set translation X parameter.
 */
void M3D_setTXMat3D(fix16 tx);
/**
 *  \brief
 *      Set translation X parameter.
 */
void M3D_setTYMat3D(fix16 ty);
/**
 *  \brief
 *      Set translation X parameter.
 */
void M3D_setTZMat3D(fix16 tz);
/**
 *  \brief
 *      Set translation parameters.
 */
void M3D_setTXYZMat3D(fix16 tx, fix16 ty, fix16 tz);
/**
 *  \brief
 *      Set translation parameters.
 */
void M3D_setTransMat3D(Vect3D_f16 *trans);

/**
 *  \brief
 *      Set rotation X parameter.
 */
void M3D_setRXMat3D(fix16 rx);
/**
 *  \brief
 *      Set rotation Y parameter.
 */
void M3D_setRYMat3D(fix16 ry);
/**
 *  \brief
 *      Set rotation Z parameter.
 */
void M3D_setRZMat3D(fix16 rz);
/**
 *  \brief
 *      Set rotation parameters.
 */
void M3D_setRXYZMat3D(fix16 rx, fix16 ry, fix16 rz);
/**
 *  \brief
 *      Set rotation parameters.
 */
void M3D_setRotMat3D(Vect3D_f16 *rot);

/**
 *  \brief
 *      Process 3D transform to specified 3D vertices buffer.
 *
 *  \param src
 *      Source 3D vertices buffer.
 *  \param dest
 *      Destination 3D vertices buffer.
 *  \param numv
 *      Number of vertices to transform.
 */
void M3D_transform3D(const Vect3D_f16 *src, Vect3D_f16 *dest, u16 numv);
//void M3D_project3D_f16_old(const Vect3D_f16 *src, Vect2D_f16 *dest, u16 numv);
//void M3D_project3D_s16_old(const Vect3D_f16 *src, Vect2D_s16 *dest, u16 numv);
/**
 *  \brief
 *      Process 2D projection to specified 3D vertices buffer.
 *
 *  \param src
 *      Source 3D vertices buffer.
 *  \param dest
 *      Destination 2D vertices buffer - fix16
 *  \param numv
 *      Number of vertices to project.
 */
void M3D_project3D_f16(const Vect3D_f16 *src, Vect2D_f16 *dest, u16 numv);
/**
 *  \brief
 *      Process 2D projection to specified 3D vertices buffer.
 *
 *  \param src
 *      Source 3D vertices buffer.
 *  \param dest
 *      Destination 2D vertices buffer - s16
 *  \param numv
 *      Number of vertices to project.
 */
void M3D_project3D_s16(const Vect3D_f16 *src, Vect2D_s16 *dest, u16 numv);


#endif // _MATHS3D_H_
