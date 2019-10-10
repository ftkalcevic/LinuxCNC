// -*- coding: utf-8 -*-
// Copyright (C) 2012 Rosen Diankov <rosen.diankov@gmail.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/** \brief  Header file for all ikfast c++ files/shared objects.

    The ikfast inverse kinematics compiler is part of OpenRAVE.

    The file is divided into two sections:
    - <b>Common</b> - the abstract classes section that all ikfast share regardless of their settings
    - <b>Library Specific</b> - the library-specific definitions, which depends on the precision/settings that the library was compiled with

    The defines are as follows, they are also used for the ikfast C++ class:

   - IKFAST_HEADER_COMMON - common classes
   - IKFAST_HAS_LIBRARY - if defined, will include library-specific functions. by default this is off
   - IKFAST_CLIBRARY - Define this linking statically or dynamically to get correct visibility.
   - IKFAST_NO_MAIN - Remove the ``main`` function, usually used with IKFAST_CLIBRARY
   - IKFAST_ASSERT - Define in order to get a custom assert called when NaNs, divides by zero, and other invalid conditions are detected.
   - IKFAST_REAL - Use to force a custom real number type for IkReal.
   - IKFAST_NAMESPACE - Enclose all functions and classes in this namespace, the ``main`` function is excluded.

 */
#include <stddef.h>

#ifndef IKFAST_HEADER_COMMON
#define IKFAST_HEADER_COMMON

/// should be the same as ikfast.__version__
#define IKFAST_VERSION 68

#define countof(x)  (sizeof((x))/(sizeof((x)[0])))

#ifdef IKFAST_REAL
    typedef IKFAST_REAL IkReal;
#else
    typedef double IkReal;
#endif

#include <linux/types.h>

/*
#ifndef __KERNEL__
    typedef int bool;
    #define true 1
    #define false 0

#endif
*/

#define DBL_EPSILON     __DBL_EPSILON__

// For a standalone binary
#ifndef IKFAST_NO_MAIN
    #define j0 math_h_j0
    #define j1 math_h_j1

    #include <math.h>
    #include <complex.h>
    #include <stdlib.h>
    #include <stdio.h>

    #undef j0
    #undef j1

    #define rtapi_exit  exit
    #define rtapi_print printf
#else
#define complex		_Complex
    #define _Complex_I	(__extension__ 1.0iF)
    #undef I
    #define I _Complex_I
    static inline double cimag( double complex z) { return __imag__ z; }
    static inline double creal( double complex z) { return __real__ z; }
    static inline double cabs( double complex z) { return __builtin_cabs(z); }

//#define INFINITY    CONCAT2(__builtin_inf, CEXT) ()
//#define isfinite(x) __builtin_expect (!isnan((x) - (x)), 1)

#define LIBGCC2_LONG_DOUBLE_TYPE_SIZE 64

# define DFtype double  
# define DCtype double complex
# define MTYPE  DFtype
# define CTYPE  DCtype
# define MODE   dc
# if LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 64
#  define CEXT  l
#  define NOTRUNC 1
# else
#  define CEXT
#  define NOTRUNC __FLT_EVAL_METHOD__ == 0 || __FLT_EVAL_METHOD__ == 1
# endif

#define CONCAT3(A,B,C)  _CONCAT3(A,B,C)
#define _CONCAT3(A,B,C) A##B##C

#define CONCAT2(A,B)    _CONCAT2(A,B)
#define _CONCAT2(A,B)   A##B



/* Helpers to make the following code slightly less gross.  */
#define COPYSIGN    CONCAT2(__builtin_copysign, CEXT)
#define FABS        CONCAT2(__builtin_fabs, CEXT)


#if NOTRUNC
# define TRUNC(x)
#else
# define TRUNC(x)   __asm__ ("" : "=m"(x) : "m"(x))
#endif


CTYPE
CONCAT3(__mul,MODE,3) (MTYPE a, MTYPE b, MTYPE c, MTYPE d)
{
  MTYPE ac, bd, ad, bc, x, y;
  CTYPE res;

  ac = a * c;
  bd = b * d;
  ad = a * d;
  bc = b * c;

  TRUNC (ac);
  TRUNC (bd);
  TRUNC (ad);
  TRUNC (bc);

  x = ac - bd;
  y = ad + bc;

  if (isnan (x) && isnan (y))
    {
      /* Recover infinities that computed as NaN + iNaN.  */
      _Bool recalc = 0;
      if (isinf (a) || isinf (b))
    {
      /* z is infinite.  "Box" the infinity and change NaNs in
         the other factor to 0.  */
      a = COPYSIGN (isinf (a) ? 1 : 0, a);
      b = COPYSIGN (isinf (b) ? 1 : 0, b);
      if (isnan (c)) c = COPYSIGN (0, c);
      if (isnan (d)) d = COPYSIGN (0, d);
          recalc = 1;
    }
     if (isinf (c) || isinf (d))
    {
      /* w is infinite.  "Box" the infinity and change NaNs in
         the other factor to 0.  */
      c = COPYSIGN (isinf (c) ? 1 : 0, c);
      d = COPYSIGN (isinf (d) ? 1 : 0, d);
      if (isnan (a)) a = COPYSIGN (0, a);
      if (isnan (b)) b = COPYSIGN (0, b);
      recalc = 1;
    }
     if (!recalc
      && (isinf (ac) || isinf (bd)
          || isinf (ad) || isinf (bc)))
    {
      /* Recover infinities from overflow by changing NaNs to 0.  */
      if (isnan (a)) a = COPYSIGN (0, a);
      if (isnan (b)) b = COPYSIGN (0, b);
      if (isnan (c)) c = COPYSIGN (0, c);
      if (isnan (d)) d = COPYSIGN (0, d);
      recalc = 1;
    }
      if (recalc)
    {
      x = INFINITY * (a * c - b * d);
      y = INFINITY * (a * d + b * c);
    }
    }

  __real__ res = x;
  __imag__ res = y;
  return res;
}


CTYPE
CONCAT3(__div,MODE,3) (MTYPE a, MTYPE b, MTYPE c, MTYPE d)
{
  MTYPE denom, ratio, x, y;
  CTYPE res;

  /* ??? We can get better behavior from logarithmic scaling instead of
     the division.  But that would mean starting to link libgcc against
     libm.  We could implement something akin to ldexp/frexp as gcc builtins
     fairly easily...  */
  if (FABS (c) < FABS (d))
    {
      ratio = c / d;
      denom = (c * ratio) + d;
      x = ((a * ratio) + b) / denom;
      y = ((b * ratio) - a) / denom;
    }
  else
    {
      ratio = d / c;
      denom = (d * ratio) + c;
      x = ((b * ratio) + a) / denom;
      y = (b - (a * ratio)) / denom;
    }

  /* Recover infinities and zeros that computed as NaN+iNaN; the only cases
     are nonzero/zero, infinite/finite, and finite/infinite.  */
  if (isnan (x) && isnan (y))
    {
      if (c == 0.0 && d == 0.0 && (!isnan (a) || !isnan (b)))
    {
      x = COPYSIGN (INFINITY, c) * a;
      y = COPYSIGN (INFINITY, c) * b;
    }
      else if ((isinf (a) || isinf (b)) && isfinite (c) && isfinite (d))
    {
      a = COPYSIGN (isinf (a) ? 1 : 0, a);
      b = COPYSIGN (isinf (b) ? 1 : 0, b);
      x = INFINITY * (a * c + b * d);
      y = INFINITY * (b * c - a * d);
    }
      else if ((isinf (c) || isinf (d)) && isfinite (a) && isfinite (b))
    {
      c = COPYSIGN (isinf (c) ? 1 : 0, c);
      d = COPYSIGN (isinf (d) ? 1 : 0, d);
      x = 0.0 * (a * c + b * d);
      y = 0.0 * (b * c - a * d);
    }
    }

  __real__ res = x;
  __imag__ res = y;
  return res;
}


#endif


/// \brief holds the solution for a single dof
struct IkSingleDOFSolution
{
    IkReal fmul, foffset; ///< joint value is fmul*sol[freeind]+foffset
    signed char freeind; ///< if >= 0, mimics another joint
    unsigned char jointtype; ///< joint type, 0x01 is revolute, 0x11 is slider
    unsigned char maxsolutions; ///< max possible indices, 0 if controlled by free index or a free joint itself
    unsigned char indices[5]; ///< unique index of the solution used to keep track on what part it came from. sometimes a solution can be repeated for different indices. store at least another repeated root
};
typedef struct IkSingleDOFSolution IkSingleDOFSolution;

static inline void IkSingleDOFSolution_Init( IkSingleDOFSolution *base, size_t count )
{
    int i;
    for ( i = 0; i < count; i++, base++ )
    {
        base->fmul = 0;
        base->foffset = 0;
        base->freeind = -1;
        base->maxsolutions = 1;
        base->indices[0] = base->indices[1] = base->indices[2] = base->indices[3] = base->indices[4] = -1;
     }
}



// Implementations of the abstract classes, user doesn't need to use them

/// \brief Default implementation of \ref IkSolutionBase
struct IkSolution 
{
    IkSingleDOFSolution _vbasesol[IKFAST_NUM_JOINTS];       ///< solution and their offsets if joints are mimiced
    int vbasesol_count;
    int _vfree[IKFAST_NUM_JOINTS];
    int vfree_count;

};
typedef struct IkSolution IkSolution;


static inline void IkSolution_Set( IkSolution *sol, const IkSingleDOFSolution *vinfos, int vinfos_count, const int *vfree, int vfree_count )
{
    int i;
    for ( i = 0; i < vinfos_count; i++ )
        sol->_vbasesol[i] = vinfos[i];
    sol->vbasesol_count = vinfos_count;
    for ( i = 0; i < vfree_count; i++ )
        sol->_vfree[i] = vfree[i];
    sol->vfree_count= vfree_count;
}

// Extract joints and free values from a solution
static inline void IkSolution_GetSolution(IkSolution *sol, IkReal* solution, int *count, const IkReal* freevalues) 
{
    size_t i;
    *count =  sol->vbasesol_count;
    for(i = 0; i < sol->vbasesol_count; ++i) {
        if( sol->_vbasesol[i].freeind < 0 )
            solution[i] = sol->_vbasesol[i].foffset;
        else {
            solution[i] = freevalues[sol->_vbasesol[i].freeind]*sol->_vbasesol[i].fmul + sol->_vbasesol[i].foffset;
            if( solution[i] > (IkReal)(3.14159265358979) ) {
                solution[i] -= (IkReal)(6.28318530717959);
            }
            else if( solution[i] < (IkReal)(-3.14159265358979) ) {
                solution[i] += (IkReal)(6.28318530717959);
            }
        }
    }
}

static inline const int* IkSolution_GetFree(IkSolution *sol, int *count ) {
    *count = sol->vfree_count;
    return sol->_vfree;
}
static inline const int GetDOF() {
    return IKFAST_NUM_DOF;
}

/*
    virtual void Validate() const {
        for(size_t i = 0; i < _vbasesol.size(); ++i) {
            if( _vbasesol[i].maxsolutions == (unsigned char)-1) {
                throw std::runtime_error("max solutions for joint not initialized");
            }
            if( _vbasesol[i].maxsolutions > 0 ) {
                if( _vbasesol[i].indices[0] >= _vbasesol[i].maxsolutions ) {
                    throw std::runtime_error("index >= max solutions for joint");
                }
                if( _vbasesol[i].indices[1] != (unsigned char)-1 && _vbasesol[i].indices[1] >= _vbasesol[i].maxsolutions ) {
                    throw std::runtime_error("2nd index >= max solutions for joint");
                }
            }
        }
    }

    virtual void GetSolutionIndices(std::vector<unsigned int>& v) const {
        v.resize(0);
        v.push_back(0);
        for(int i = (int)_vbasesol.size()-1; i >= 0; --i) {
            if( _vbasesol[i].maxsolutions != (unsigned char)-1 && _vbasesol[i].maxsolutions > 1 ) {
                for(size_t j = 0; j < v.size(); ++j) {
                    v[j] *= _vbasesol[i].maxsolutions;
                }
                size_t orgsize=v.size();
                if( _vbasesol[i].indices[1] != (unsigned char)-1 ) {
                    for(size_t j = 0; j < orgsize; ++j) {
                        v.push_back(v[j]+_vbasesol[i].indices[1]);
                    }
                }
                if( _vbasesol[i].indices[0] != (unsigned char)-1 ) {
                    for(size_t j = 0; j < orgsize; ++j) {
                        v[j] += _vbasesol[i].indices[0];
                    }
                }
            }
        }
    }

*/

/// \brief Default implementation of \ref IkSolutionListBase
struct IkSolutionList 
{
#define IKFAST_MAX_SOLUTIONS    16
    IkSolution _listsolutions[IKFAST_MAX_SOLUTIONS];
    int _count;
};
typedef struct IkSolutionList IkSolutionList;

static inline void IkSolutionList_Init( IkSolutionList *list )
{
    list->_count = 0;
}


static inline size_t IkSolutionList_AddSolution( IkSolutionList *list,
                                          const IkSingleDOFSolution * vinfos, int vinfos_count, 
                                          const int *vfree, int vfree_count )
{
    size_t index = list->_count;
    if ( index < IKFAST_MAX_SOLUTIONS )
    {
        IkSolution_Set( list->_listsolutions+index, vinfos, vinfos_count, vfree, vfree_count );
        list->_count++;
    }
    else
    {
        rtapi_print("Exceeded max solutions\n");
    }
    return index;
}

static inline IkSolution * IkSolutionList_GetSolution(IkSolutionList *list, size_t index)
{
    if( index >= list->_count ) 
    {
        // assert throw std::runtime_error("GetSolution index is invalid");
    }
    return list->_listsolutions+index;
}

static inline size_t IkSolutionList_GetNumSolutions(IkSolutionList *list) 
{
    return list->_count;
}

static inline void IkSolutionList_Clear( IkSolutionList *list ) 
{
    list->_count = 0;
}


#endif // OPENRAVE_IKFAST_HEADER

// The following code is dependent on the C++ library linking with.
#ifdef IKFAST_HAS_LIBRARY

// defined when creating a shared object/dll
#ifdef IKFAST_CLIBRARY
#ifdef _MSC_VER
#define IKFAST_API extern "C" __declspec(dllexport)
#else
#define IKFAST_API extern "C"
#endif
#else
#define IKFAST_API
#endif
/// \brief Computes the end effector coordinates given the joint values. This function is used to double check ik.
IKFAST_API void ComputeFk(const IkReal* joints, IkReal* eetrans, IkReal* eerot);

/** \brief Computes all IK solutions given a end effector coordinates and the free joints.

   - ``eetrans`` - 3 translation values. For iktype **TranslationXYOrientation3D**, the z-axis is the orientation.
   - ``eerot``
   - For **Transform6D** it is 9 values for the 3x3 rotation matrix.
   - For **Direction3D**, **Ray4D**, and **TranslationDirection5D**, the first 3 values represent the target direction.
   - For **TranslationXAxisAngle4D**, **TranslationYAxisAngle4D**, and **TranslationZAxisAngle4D** the first value represents the angle.
   - For **TranslationLocalGlobal6D**, the diagonal elements ([0],[4],[8]) are the local translation inside the end effector coordinate system.
 */
IKFAST_API int ComputeIk(const IkReal* eetrans, const IkReal* eerot, const IkReal* pfree, IkSolutionList* solutions);

// \brief returns the number of free parameters users has to set apriori
IKFAST_API int GetNumFreeParameters();

/// \brief the indices of the free parameters indexed by the chain joints
IKFAST_API int* GetFreeParameters();

/// \brief the total number of indices of the chain
IKFAST_API int GetNumJoints();

/// \brief the size in bytes of the configured number type
IKFAST_API int GetIkRealSize();

/// \brief the ikfast version used to generate this file
IKFAST_API const char* GetIkFastVersion();

/// \brief the ik type ID
IKFAST_API int GetIkType();

/// \brief a hash of all the chain values used for double checking that the correct IK is used.
IKFAST_API const char* GetKinematicsHash();


#endif // IKFAST_HAS_LIBRARY
