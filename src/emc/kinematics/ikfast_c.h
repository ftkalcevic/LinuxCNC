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
//#include <vector>
//#include <list>
//#include <stdexcept>
#include <math.h>
#include <stdlib.h>
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

typedef int bool;
const int true = 1;
const int false = 0;

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

inline void IkSingleDOFSolution_Init( IkSingleDOFSolution *base, size_t count )
{
    int i;
    for ( i = 0; i < count; i++, base++ )
    {
        base->fmul = 0;
        base->foffset = 0;
        base->freeind = -1;
        base->maxsolutions = 1;
    }
}



// Implementations of the abstract classes, user doesn't need to use them

/// \brief Default implementation of \ref IkSolutionBase
struct IkSolution 
{
    IkSingleDOFSolution _vbasesol[IKFAST_NUM_DOF];       ///< solution and their offsets if joints are mimiced
    int _vfree[IKFAST_NUM_DOF];

};
typedef struct IkSolution IkSolution;


inline void IkSolution_Set( IkSolution *sol, const IkSingleDOFSolution *vinfos, int vinfos_count, const int *vfree, int vfree_count )
{
    int i;
    for ( i = 0; i < vinfos_count; i++ )
        sol->_vbasesol[i] = vinfos[i];
    for ( i = 0; i < vfree_count; i++ )
        sol->_vfree[i] = vfree[i];
}

// Extract joints and free values from a solution
inline void IkSolution_GetSolution(IkSolution *sol, IkReal* solution, const IkReal* freevalues) 
{
    size_t i;
    for(i = 0; i < countof(sol->_vbasesol); ++i) {
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

inline const int* GetFree(IkSolution *sol) {
    return sol->_vfree;
}
inline const int GetDOF() {
    return 4;
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

inline void IkSolutionList_Init( IkSolutionList *list )
{
    list->_count = 0;
}


inline size_t IkSolutionList_AddSolution( IkSolutionList *list,
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
        // Assert
    }
    return index;
}

inline IkSolution * IkSolutionList_GetSolution(IkSolutionList *list, size_t index)
{
    if( index >= list->_count ) 
    {
        // assert throw std::runtime_error("GetSolution index is invalid");
    }
    return list->_listsolutions+index;
}

inline size_t IkSolutionList_GetNumSolutions(IkSolutionList *list) 
{
    return list->_count;
}

inline void IkSolutionList_Clear( IkSolutionList *list ) 
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

/** \brief Computes all IK solutions given a end effector coordinates and the free joints.

   - ``eetrans`` - 3 translation values. For iktype **TranslationXYOrientation3D**, the z-axis is the orientation.
   - ``eerot``
   - For **Transform6D** it is 9 values for the 3x3 rotation matrix.
   - For **Direction3D**, **Ray4D**, and **TranslationDirection5D**, the first 3 values represent the target direction.
   - For **TranslationXAxisAngle4D**, **TranslationYAxisAngle4D**, and **TranslationZAxisAngle4D** the first value represents the angle.
   - For **TranslationLocalGlobal6D**, the diagonal elements ([0],[4],[8]) are the local translation inside the end effector coordinate system.
 */
IKFAST_API int ComputeIk(const IkReal* eetrans, const IkReal* eerot, const IkReal* pfree, IkSolutionList* solutions);

/// \brief Computes the end effector coordinates given the joint values. This function is used to double check ik.
IKFAST_API void ComputeFk(const IkReal* joints, IkReal* eetrans, IkReal* eerot);

/// \brief returns the number of free parameters users has to set apriori
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

#ifdef IKFAST_NAMESPACE
}
#endif

#endif // IKFAST_HAS_LIBRARY
