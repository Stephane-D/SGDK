/**
 *  \file maths3D.h
 *  \brief 3D math engine.
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides 3D transformation methods :<br>
 * - translation X, Y, Z<br>
 * - rotation X, Y, Z<br>
 * - one directionnal light<br>
 * - 2D projection<br>
 *<br>
 * Can transform (including 2D projection) about ~10000 vertices / seconde.
 */

#ifndef _MATHS3D_H_
#define _MATHS3D_H_


/**
 *  \struct Translation3D
 *      3D translation informations object - f16 (fix16) type.
 */
typedef Vect3D_f16 Translation3D;

/**
 *  \struct Rotation3D
 *      3D rotation informations object - f16 (fix16) type.
 */
typedef Vect3D_f16 Rotation3D;


/**
 *  \struct Transformation3D
 *      3D transformation object - f16 (fix16) type.<br>
 *      This object define the global 3D transformation informations and associated cached data.<br>
 *      If rotation information is modified the rebuildMat flag should be set to 1.<br>
 *      Rotation and translation objects are reference so don't forget to set them.
 */
typedef struct
{
    u16 rebuildMat;
    Translation3D *translation;
    Rotation3D *rotation;
    Mat3D_f16 mat;
    Mat3D_f16 matInv;
    Vect3D_f16 cameraInv;
    Vect3D_f16 lightInv;
} Transformation3D;

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
void M3D_setCamDistance(fix16 value);
/**
 *  \brief
 *      Set light direction vector.
 */
void M3D_setLightXYZ(fix16 x, fix16 y, fix16 z);
/**
 *  \brief
 *      Set light direction vector.
 */
void M3D_setLight(Vect3D_f16 *value);

/**
 *  \brief
 *      Reset the specified rotation object.
 */
void M3D_resetTransform(Transformation3D *t);

/**
 *  \brief
 *      Set translation and rotation objects to the specified transformation object.
 */
void M3D_setTransform(Transformation3D *tr, Translation3D *t, Rotation3D *r);

/**
 *  \brief
 *      Set translation parameters to the specified transformation object.
 */
void M3D_setTranslation(Transformation3D *t, fix16 x, fix16 y, fix16 z);
/**
 *  \brief
 *      Set rotation parameters to the specified #Transformation3D object.<br>
 *      Be careful, value is not given in radiant.<br>
 *      [-8..+8] range correspond to radian [-PI..+PI] range.
 */
void M3D_setRotation(Transformation3D *t, fix16 x, fix16 y, fix16 z);

/**
 *  \brief
 *      Combine the specified right and left #Transformation3D objects and store result in <code>result</code>.
 *      result cannot be the same transformation object as left or right.
 *
 *  \param left
 *      Left #Transformation3D object.
 *  \param right
 *      Right #Transformation3D object.
 *  \param result
 *      Result #Transformation3D object.
 */
void M3D_combineTransform(Transformation3D *left, Transformation3D *right, Transformation3D *result);
/**
 *  \brief
 *      Combine the specified left #Translation3D and right #Transformation3D and store result in <code>result</code>.<br>
 *      right and result transformation object can be the same.
 *
 *  \param left
 *      Left #Transformation3D object.
 *  \param right
 *      Right #Transformation3D object.
 *  \param result
 *      Result #Transformation3D object.
 */
void M3D_combineTranslationLeft(Translation3D *left, Transformation3D *right, Transformation3D *result);
/**
 *  \brief
 *      Combine the specified left #Transformation3D with right #Translation3D and store result in <code>result</code>.<br>
 *      left and result transformation object can be the same.
 *
 *  \param left
 *      Left #Transformation3D object.
 *  \param right
 *      Right #Translation3D object.
 *  \param result
 *      Result #Transformation3D object.
 */
void M3D_combineTranslationRight(Transformation3D *left, Translation3D *right, Transformation3D *result);

/**
 *  \brief
 *      Build the transformation matrix of the specified #Transformation3D object.<br>
 *      This also rebuild cached informations as inverse transformation matrix, inverse camera view...
 *
 *  \param t
 *      #Transformation3D object.
 */
void M3D_buildMat3D(Transformation3D *t);
/**
 *  \brief
 *      Build the transformation matrix of the specified transformation object.
 *      Only rebuild the transformation matrix (faster), cached infos as inverse matrix are not rebuild.
 *
 *  \param t
 *      Transformation object.
 */
void M3D_buildMat3DOnly(Transformation3D *t);
/**
 *  \brief
 *      Only rebuild the cached infos as inverse matrix, inverse camera view...
 *
 *  \param t
 *      Transformation object.
 */
void M3D_buildMat3DExtras(Transformation3D *t);

/**
 *  \brief
 *      Process 3D translation only to specified 3D vertices buffer.
 *
 *  \param t
 *      Transformation object containing translation parameter.
 *  \param vertices
 *      3D vertices buffer to translate.
 *  \param numv
 *      Number of vertices to translate.
 */
void M3D_translate(Transformation3D *t, Vect3D_f16 *vertices, u16 numv);
/**
 *  \brief
 *      Process 3D rotation only to specified 3D vertices buffer.
 *
 *  \param t
 *      Transformation object containing rotation parameter.
 *  \param src
 *      Source 3D vertices buffer.
 *  \param dest
 *      Destination 3D vertices buffer.
 *  \param numv
 *      Number of vertices to rotate.
 */
void M3D_rotate(Transformation3D *t, const Vect3D_f16 *src, Vect3D_f16 *dest, u16 numv);
/**
 *  \brief
 *      Process 3D inverse rotation only to specified 3D vertex.
 *
 *  \param t
 *      Transformation object containing rotation parameter.
 *  \param src
 *      Source 3D vertex.
 *  \param dest
 *      Destination 3D vertex.
 */
void M3D_rotateInv(Transformation3D *t, const Vect3D_f16 *src, Vect3D_f16 *dest);
/**
 *  \brief
 *      Process 3D transform (rotation and translation) to specified 3D vertices buffer.
 *
 *  \param t
 *      Transformation object containing rotation and translation parameters.
 *  \param src
 *      Source 3D vertices buffer.
 *  \param dest
 *      Destination 3D vertices buffer.
 *  \param numv
 *      Number of vertices to transform.
 */
void M3D_transform(Transformation3D *t, const Vect3D_f16 *src, Vect3D_f16 *dest, u16 numv);

/**
 *  \brief
 *      Process 2D projection to specified 3D vertices buffer (fix16 version).
 *
 *  \param src
 *      Source 3D vertices buffer.
 *  \param dest
 *      Destination 2D vertices buffer - fix16 format
 *  \param numv
 *      Number of vertices to project.
 */
void M3D_project_f16(const Vect3D_f16 *src, Vect2D_f16 *dest, u16 numv);
/**
 *  \brief
 *      Process 2D projection to specified 3D vertices buffer (s16 version).
 *
 *  \param src
 *      Source 3D vertices buffer.
 *  \param dest
 *      Destination 2D vertices buffer - s16 format
 *  \param numv
 *      Number of vertices to project.
 */
void M3D_project_s16(const Vect3D_f16 *src, Vect2D_s16 *dest, u16 numv);


#endif // _MATHS3D_H_
