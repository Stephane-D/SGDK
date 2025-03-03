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


// 3D base structutes

/**
 *  \brief
 *      3D Vector structure - u16 type.
 */
typedef struct
{
    u16 x;
    u16 y;
    u16 z;
} Vect3D_u16;

/**
 *  \brief
 *      3D Vector structure - s16 type.
 */
typedef struct
{
    s16 x;
    s16 y;
    s16 z;
} Vect3D_s16;

/**
 *  \brief
 *      3D Vector structure - u32 type.
 */
typedef struct
{
    u32 x;
    u32 y;
    u32 z;
} Vect3D_u32;

/**
 *  \brief
 *      3D Vector structure - s32 type.
 */
typedef struct
{
    s32 x;
    s32 y;
    s32 z;
} Vect3D_s32;

/**
 *  \brief
 *      3D Vector structure - f16 (fix16) type.
 */
typedef struct
{
    fix16 x;
    fix16 y;
    fix16 z;
} Vect3D_f16;

/**
 *  \brief
 *      3D Vector structure - f32 (fix32) type.
 */
typedef struct
{
    fix32 x;
    fix32 y;
    fix32 z;
} Vect3D_f32;

/**
 *  \brief
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
 *      3x3 Matrice structure - f32 (fix32) type.<br>
 *      Internally uses 3 3D vectors.
 */
typedef struct
{
    Vect3D_f32 a;
    Vect3D_f32 b;
    Vect3D_f32 c;
} Mat3D_f32;


// 4D base structures

/**
 *  \brief
 *      4D Vector structure - f16 (fix16) type.
 */
typedef struct
{
    fix16 x;
    fix16 y;
    fix16 z;
    fix16 w;
} Vect4D_f16;

/**
 *  \brief
 *      4D Vector structure - f32 (fix32) type.
 */
typedef struct
{
    fix32 x;
    fix32 y;
    fix32 z;
    fix32 w;
} Vect4D_f32;

/**
 *  \brief
 *      4x4 Matrice structure - f16 (fix16) type.<br>
 *      Internally uses 4 4D vectors.
 */
typedef struct
{
    Vect4D_f16 a;
    Vect4D_f16 b;
    Vect4D_f16 c;
    Vect4D_f16 d;
} Mat4D_f16;

/**
 *  \brief
 *      4x4 Matrice structure - f32 (fix32) type.<br>
 *      Internally uses 4 4D vectors.
 */
typedef struct
{
    Vect4D_f32 a;
    Vect4D_f32 b;
    Vect4D_f32 c;
    Vect4D_f32 d;
} Mat4D_f32;


// short alias

/**
 *  \brief alias for Vect3D_u16
 */
typedef Vect3D_u16 V3u16;
/**
 *  \brief alias for Vect3D_s16
 */
typedef Vect3D_s16 V3s16;
/**
 *  \brief alias for Vect3D_u32
 */
typedef Vect3D_u32 V3u32;
/**
 *  \brief alias for Vect3D_s32
 */
typedef Vect3D_s32 V3s32;
/**
 *  \brief alias for Vect3D_f16
 */
typedef Vect3D_f16 V3f16;
/**
 *  \brief alias for Vect3D_f32
 */
typedef Vect3D_f32 V3f32;

/**
 *  \brief alias for Vect4D_f16
 */
typedef Vect4D_f16 V4f16;
/**
 *  \brief alias for Vect4D_f32
 */
typedef Vect4D_f32 V4f32;

/**
 *  \brief alias for Mat3D_f16
 */
typedef Mat3D_f16 M3f16;
/**
 *  \brief alias for Mat3D_f32
 */
typedef Mat3D_f32 M3f32;
/**
 *  \brief alias for Mat4D_f16
 */
typedef Mat4D_f16 M4f16;
/**
 *  \brief alias for Mat4D_f32
 */
typedef Mat4D_f32 M4f32;


// advanced 3D structures

/**
 *  \brief
 *      Structure hosting settings / context for the 3D transform engine.
 */
typedef struct
{
    V2u16 viewport;
    fix16 camDist;
    V3f16 light;
    u16 lightEnabled;
} Context3D;

/**
 *  \brief
 *      3D translation informations object - f16 (fix16) type.
 */
typedef V3f16 Translation3D;

/**
 *  \brief
 *      3D rotation informations object (angles are stored in degree) - f16 (fix16) type.
 */
typedef V3f16 Rotation3D;

/**
 *  \brief
 *      3D transformation object - f16 (fix16) type.<br>
 *      This object define the global 3D transformation informations and associated cached data.<br>
 *      If rotation information is modified the rebuildMat flag should be set to 1.<br>
 *      Rotation and translation objects are reference so don't forget to set them.
 */
typedef struct
{
    u16 rebuildMat;
    Translation3D* translation;
    Rotation3D* rotation;
    M3f16 mat;
    M3f16 matInv;
    V3f16 cameraInv;
    V3f16 lightInv;
} Transformation3D;

/**
 *  \brief
 *      Reset math 3D engine (reset matrices and transformation parameters mainly).
 */
void M3D_reset(void);

/**
 *  \brief
 *      Enable or disable light transformation calculation.
 */
void M3D_setLightEnabled(u16 enabled);
/**
 *  \brief
 *      Get light transformation calculation enabled flag.
 */
u16  M3D_getLightEnabled(void);

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
void M3D_setLight(V3f16* value);

/**
 *  \brief
 *      Reset the specified Transformation3D object.
 */
void M3D_resetTransform(Transformation3D* t);

/**
 *  \brief
 *      Set translation and rotation objects to the specified transformation object.
 */
void M3D_setTransform(Transformation3D* tr, Translation3D* t, Rotation3D *r);

/**
 *  \brief
 *      Set translation parameters to the specified transformation object.
 */
void M3D_setTranslation(Transformation3D* t, fix16 x, fix16 y, fix16 z);
/**
 *  \brief
 *      Set rotation parameters to the specified #Transformation3D object.<br>
 *      x, y, z angle values are given in degree (fix16)
 */
void M3D_setRotation(Transformation3D* t, fix16 x, fix16 y, fix16 z);

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
void M3D_combineTransform(Transformation3D* left, Transformation3D* right, Transformation3D* result);
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
void M3D_combineTranslationLeft(Translation3D* left, Transformation3D* right, Transformation3D* result);
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
void M3D_combineTranslationRight(Transformation3D* left, Translation3D* right, Transformation3D* result);

/**
 *  \brief
 *      Build the transformation matrix of the specified #Transformation3D object.<br>
 *      This also rebuild cached informations as inverse transformation matrix, inverse camera view...
 *
 *  \param t
 *      #Transformation3D object.
 */
void M3D_buildMat3D(Transformation3D* t);
/**
 *  \brief
 *      Build the transformation matrix of the specified transformation object.
 *      Only rebuild the transformation matrix (faster), cached infos as inverse matrix are not rebuild.
 *
 *  \param t
 *      Transformation object.
 */
void M3D_buildMat3DOnly(Transformation3D* t);
/**
 *  \brief
 *      Only rebuild the cached infos as inverse matrix, inverse camera view...
 *
 *  \param t
 *      Transformation object.
 */
void M3D_buildMat3DExtras(Transformation3D* t);

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
void M3D_translate(Transformation3D* t, V3f16* vertices, u16 numv);
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
void M3D_rotate(Transformation3D* t, const V3f16* src, V3f16* dest, u16 numv);
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
void M3D_rotateInv(Transformation3D* t, const V3f16* src, V3f16* dest);
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
void M3D_transform(Transformation3D* t, const V3f16* src, V3f16* dest, u16 numv);

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
void M3D_project_f16(const V3f16* src, V2f16* dest, u16 numv);
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
void M3D_project_s16(const V3f16* src, V2s16* dest, u16 numv);


#endif // _MATHS3D_H_
