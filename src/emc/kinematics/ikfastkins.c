/********************************************************************
* Description: ikfastkins
*   Kinematics for an ikfast IK module (see wiki for details)
*
*   Derived from a work by Fred Proctor,
*   changed to work with emc2 and HAL
*
* Adapting Author: Alex Joni
* License: GPL Version 2
* System: Linux
*    
*******************************************************************

  These are the forward and inverse kinematic functions for a general
  serial-link manipulator. Thanks to Herman Bruyninckx and John
  Hallam at http://www.roble.info/ for this.

  The functions are general enough to be configured for any serial
  configuration.  
  The kinematics use Denavit-Hartenberg definition for the joint and
  links. The DH definitions are the ones used by John J Craig in
  "Introduction to Robotics: Mechanics and Control"
  The parameters for the manipulator are defined by hal pins.
  Currently the type of the joints is hardcoded to ANGULAR, although 
  the kins support both ANGULAR and LINEAR axes.
  
  TODO:
    * make number of joints a loadtime parameter
    * add HAL pins for all settable parameters, including joint type: ANGULAR / LINEAR
    * add HAL pins for debug data (num_iterations)
    * add HAL pins for ULAPI compiled version
*/

// generated code uses variables j0, j1 which clash with std math.h
// library functions j0() and j1().  We don't use the functions so we
// just rename the std lib functions.
#define j0 math_j0
#define j1 math_j1

#include "rtapi_math.h"
#ifdef SIM
#include <complex.h>
#include <string.h>
#endif
#include "gotypes.h"		/* go_result, go_integer */
#include "gomath.h"		/* go_pose */
#include "posemath.h"
#include "kinematics.h"

#undef j0
#undef j1

#ifdef RTAPI
#include "rtapi.h"
#include "rtapi_app.h"
#endif

//#define RTAPI_PRINT_DEBUG
#ifndef RTAPI_PRINT_DEBUG
#define RTAPI_PRINT if (0) rtapi_print
#endif

#include "hal.h"
#include "emcmotcfg.h"

#define IKFAST_NO_MAIN
#define IKFAST_HAS_LIBRARY
// simple.robot.xml
#include "ikfast62.Translation3D.0_1_2_f3_4.inc"
//#include "ikfast62.TranslationZAxisAngle4D.0_1_2_3_f4.inc"
/// barretwam.robot.xml
//#include "ikfast62.Transform6D.0_1_3_4_5_6_f2.inc"


static IkReal last_eerot[9];
static int no_result_count;

#ifdef __KERNEL__
    #define ABS(x)          ((x)>=0?(x):(-(x)))
    #define INTDOUBLE(x)    (x)<0?"-":"", ((int)(ABS(x))), (((int)((ABS(x))*1000))%1000)
    #define DFMT            "%s%d.%03d"
#else
    #define INTDOUBLE(x)    x
    #define DFMT            "%f"
#endif

static struct _haldata
{
    hal_float_t *time_min;
    hal_float_t *time_max;
    hal_float_t *time_avg;
    hal_float_t *time_last;
} *haldata;
static bool bfirst;
static double time_sum;
static unsigned long time_count;


/* main function called by emc2 for forward Kins */
int kinematicsForward(const double *joint, 
                      EmcPose * world, 
                      const KINEMATICS_FORWARD_FLAGS * fflags, 
                      KINEMATICS_INVERSE_FLAGS * iflags) 
{
    IkReal j[IKFAST_NUM_JOINTS], eetrans[3], eerot[9];
    int i;

    // Convert linuxcnc degrees to ikfast radians
    for ( i = 0; i < IKFAST_NUM_JOINTS; i++ )
        j[i] = joint[i]/180.0*M_PI;

    ComputeFk(j, eetrans, eerot);

    memcpy( last_eerot, eerot, sizeof(last_eerot) );
    // Convert from ikfast m to linuxcnc mm
    world->tran.x = eetrans[0]*1000.0;
    world->tran.y = eetrans[1]*1000.0;
    world->tran.z = eetrans[2]*1000.0;

#if IKFAST_NUM_FREE_VARS > 0
    world->a = joint[GetFreeParameters()[0]];
#endif
#if IKFAST_NUM_FREE_VARS > 1
    world->b = joint[GetFreeParameters()[1]];
#endif
#if IKFAST_NUM_FREE_VARS > 2
    world->c = joint[GetFreeParameters()[2]];
#endif
#if IKFAST_NUM_FREE_VARS > 3
    world->u = joint[GetFreeParameters()[3]];
#endif
#if IKFAST_NUM_FREE_VARS > 4
    world->v = joint[GetFreeParameters()[4]];
#endif
#if IKFAST_NUM_FREE_VARS > 5
    world->w = joint[GetFreeParameters()[5]];
#endif
  
    RTAPI_PRINT("ikfastkins::kinematicsForward(joints: " ); for ( i = 0; i < IKFAST_NUM_JOINTS; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(joint[i]) ); RTAPI_PRINT("\n");
    RTAPI_PRINT("eerot: " ); for ( i = 0; i < 9; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(eerot[i]) ); RTAPI_PRINT("\n");
    RTAPI_PRINT("eetrans: " ); for ( i = 0; i < 3; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(eetrans[i]) ); RTAPI_PRINT("\n");
	RTAPI_PRINT("world: "DFMT" "DFMT" "DFMT" )\n", INTDOUBLE(world->tran.x), INTDOUBLE(world->tran.y), INTDOUBLE(world->tran.z) );
    
    return 0;
}


int kinematicsInverse(const EmcPose * world,
		      double *joints,
		      const KINEMATICS_INVERSE_FLAGS * iflags,
		      KINEMATICS_FORWARD_FLAGS * fflags)
{
    //return GO_RESULT_OK;
    IkReal jfree[IKFAST_NUM_FREE_VARS], eerot[9], eetrans[3];
    int i;
    static IkSolutionList solutions;
    bool bSuccess;

	// map free joints to axis a->
#if IKFAST_NUM_FREE_VARS > 0
    jfree[0] = world->a/180*M_PI;
#endif
#if IKFAST_NUM_FREE_VARS > 1
    jfree[1] = world->b/180*M_PI;
#endif
#if IKFAST_NUM_FREE_VARS > 2
    jfree[2] = world->c/180*M_PI;
#endif
#if IKFAST_NUM_FREE_VARS > 3
    jfree[3] = world->u/180*M_PI;
#endif
#if IKFAST_NUM_FREE_VARS > 4
    jfree[4] = world->v/180*M_PI;
#endif
#if IKFAST_NUM_FREE_VARS > 5
    jfree[5] = world->w/180*M_PI;
#endif

    // Use the last value eerot
    memcpy( eerot, last_eerot, sizeof(last_eerot) );

    // convert linuxcnc mm to ikfast m
    eetrans[0] = world->tran.x/1000.0;
    eetrans[1] = world->tran.y/1000.0;
    eetrans[2] = world->tran.z/1000.0;


	RTAPI_PRINT("ikfastkins::kinematicsInverse(world: "DFMT" "DFMT" "DFMT" "DFMT"\n", INTDOUBLE(world->tran.x), INTDOUBLE(world->tran.y), INTDOUBLE(world->tran.z), INTDOUBLE(world->a) );
    RTAPI_PRINT("beforejoints: " ); for ( i = 0; i < IKFAST_NUM_JOINTS; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(joints[i]) ); RTAPI_PRINT("\n");
    RTAPI_PRINT("eetrans: " ); for ( i = 0; i < 3; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(eetrans[i]) ); RTAPI_PRINT("\n");
    RTAPI_PRINT("eerot: " ); for ( i = 0; i < 9; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(eerot[i]) ); RTAPI_PRINT("\n");

    long long tstart = rtapi_get_time();
    IkSolutionList_Init( &solutions );
    bSuccess = ComputeIk(eetrans, eerot, jfree, &solutions);
    long long tend = rtapi_get_time();
    double t = (tend-tstart)/1000.0;
    RTAPI_PRINT("bSuccess=%d time="DFMT"ms\n", bSuccess, INTDOUBLE(t));
    *(haldata->time_last) = t;
    time_sum += t;
    time_count++;
    *(haldata->time_avg) = time_sum / time_count;
    if ( bfirst || t < *(haldata->time_min) ) *(haldata->time_min) = t;
    if ( bfirst || t > *(haldata->time_max) ) *(haldata->time_max) = t;
    bfirst = false;

    if ( bSuccess )
    {
        int solutionCount = IkSolutionList_GetNumSolutions(&solutions);
        int bestSolution = -1;
        double bestSolutionDiff = DBL_MAX;
        RTAPI_PRINT("solns: %d - ", solutionCount );
        {
            int s;
            for ( s = 0; s < solutionCount; s++ )
            {
                bool outOfRange = false;
                IkSolution *sol = IkSolutionList_GetSolution(&solutions,s);
                IkReal solvalues[IKFAST_NUM_JOINTS];
                double err=0;
                size_t j;
                int solvalues_count;
                IkSolution_GetSolution(sol,solvalues,&solvalues_count,jfree);
                RTAPI_PRINT("- joints: ");
                for( j = 0; j < solvalues_count; ++j)
                {
                    // convert from ikfast radians to linuxcnc degrees
                    solvalues[j] *= 180 / M_PI;
                    // Check joint ranges
                    if ( solvalues[j] < JointInfo[j].limitMin ||
                         solvalues[j] > JointInfo[j].limitMax )
                    {
                        outOfRange = true;
                        RTAPI_PRINT("*" );
                    }
                    else
                    {
                        double diff = joints[j] - solvalues[j];
                        while ( diff < 180 ) diff += 360;
                        while ( diff > 180 ) diff -= 360;
                        err += diff*diff;
                    }
                    RTAPI_PRINT(""DFMT" ", INTDOUBLE(solvalues[j]) );
                }
                if ( !outOfRange && err < bestSolutionDiff )
                {
                    bestSolution = s;
                    bestSolutionDiff = err;
                }
            }
            RTAPI_PRINT(")\n" );
        }
        if ( bestSolution >= 0 )
        {
            size_t j;
            IkSolution *sol = IkSolutionList_GetSolution(&solutions,bestSolution);
            IkReal solvalues[IKFAST_NUM_JOINTS];
            int solvalues_count;
            RTAPI_PRINT( "bestSolutionDiff = "DFMT"\n", INTDOUBLE(bestSolutionDiff) );
            IkSolution_GetSolution(sol,solvalues, &solvalues_count, jfree);
            for( j = 0; j < solvalues_count; ++j)
            {
                joints[j] = solvalues[j] * 180 / M_PI;
            }
            RTAPI_PRINT("afterjoints:  " ); for ( i = 0; i < IKFAST_NUM_JOINTS; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(joints[i]) ); RTAPI_PRINT("\n");
            no_result_count = 0;
            return GO_RESULT_OK;
        }
        else
        {
            if ( no_result_count == 0 )
            {
                rtapi_print("ikfastkins: All solutions exceed joint limits\n");
            }
            no_result_count++;
            return GO_RESULT_ERROR;
        }
	}
    
	if ( no_result_count == 0 )
    {
        rtapi_print("ikfastkins: no solution\n");
    }
    no_result_count++;
    return GO_RESULT_ERROR;
}

int kinematicsHome(EmcPose * world,
    double *joint,
    KINEMATICS_FORWARD_FLAGS * fflags, KINEMATICS_INVERSE_FLAGS * iflags)
{
    /* use joints, set world */
    return kinematicsForward(joint, world, fflags, iflags);
}


KINEMATICS_TYPE kinematicsType()
{
    return KINEMATICS_BOTH;
}

#ifdef RTAPI

EXPORT_SYMBOL(kinematicsType);
EXPORT_SYMBOL(kinematicsForward);
EXPORT_SYMBOL(kinematicsInverse);
MODULE_LICENSE("GPL");

int comp_id;


int rtapi_app_main(void)
{
	RTAPI_PRINT("Starting ikfastkins\n");
#define COMPONENT_NAME "ikfastkins"
    comp_id = hal_init(COMPONENT_NAME);
    if (comp_id < 0)
	    return comp_id;

    //if ( GetNumFreeParameters() > 0 )
    //{
    //    int i;
    //    haldata = hal_malloc(sizeof(hal_float_t *)*GetNumFreeParameters());
    //    
    //    // Create parameters to feed the free joints
    //    for ( i = 0; i < GetNumFreeParameters(); i++ )
    //    {
    //        int index = GetFreeParameters()[i];
    //        int result = hal_pin_float_newf( HAL_IO, haldata+i, comp_id,
    //                                         "%s.joint%d", COMPONENT_NAME, index );
    //    }
    //}
    haldata = (struct _haldata *)hal_malloc(sizeof(struct _haldata));
    hal_pin_float_newf( HAL_OUT, &haldata->time_min, comp_id, "%s.time-min", COMPONENT_NAME );
    hal_pin_float_newf( HAL_OUT, &haldata->time_max, comp_id, "%s.time-max", COMPONENT_NAME );
    hal_pin_float_newf( HAL_OUT, &haldata->time_avg, comp_id, "%s.time-avg", COMPONENT_NAME );
    hal_pin_float_newf( HAL_OUT, &haldata->time_last, comp_id, "%s.time-last", COMPONENT_NAME );
    *(haldata->time_min) = 0;
    *(haldata->time_max) = 0;
    *(haldata->time_avg) = 0;
    *(haldata->time_last) = 0;
    bfirst = true;
    time_sum = 0;
    time_count = 0;
    no_result_count = 0;

    hal_ready(comp_id);
	RTAPI_PRINT("ikfastkins ready\n");
    return 0;
}


void rtapi_app_exit(void)
{
    hal_exit(comp_id);
}
#endif

