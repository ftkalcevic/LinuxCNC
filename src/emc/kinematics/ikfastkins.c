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
#endif
#include <string.h>
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
//#define RTAPI_PRINT if (0) rtapi_print
#define RTAPI_PRINT printf
#endif

#include "hal.h"
#include "emcmotcfg.h"

#define IKFAST_NO_MAIN
#define IKFAST_HAS_LIBRARY

#include "ikfast62.Transform6D.0_1_2_3_4_5_f6.inc"
//#include "ikfast62.Translation3D.0_1_2_f3_4_5_6.inc"

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

#define MAX_SOLUTION_ERROR  300000

static struct _haldata
{
    hal_bit_t *teleop_mode;
    hal_float_t *time_min;
    hal_float_t *time_max;
    hal_float_t *time_avg;
    hal_float_t *time_last;
    hal_float_t *gripper_offset;
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

    for ( i = 0; i < IKFAST_NUM_JOINTS; i++ )
        if ( JointInfo[i].jointType == 1 )
            j[i] = joint[i]/180.0*M_PI;     // Convert linuxcnc degrees to ikfast radians
        else 
            j[i] = joint[i]/1000.0;         // linear axis cnc mm to m

    j[6] = *(haldata->gripper_offset)/1000.0;

    ComputeFk(j, eetrans, eerot);

    if ( ! *(haldata->teleop_mode) )
    {
        // joint mode - remember end effector rotation
        memcpy( last_eerot, eerot, sizeof(last_eerot) );
    }

    // Convert from ikfast m to linuxcnc mm
    world->tran.x = eetrans[0]*1000.0;
    world->tran.y = eetrans[1]*1000.0;
    world->tran.z = eetrans[2]*1000.0;

    IkReal e1,e2,e3;
    if  ( eerot[6] == 1 || eerot[6] == -1 )
    {
        e3 = 0;
        IkReal delta = atan2(eerot[3],eerot[6]);
        if ( eerot[6] == -1 )
        {
            e2 = M_PI/2;
            e1 = e3 + delta;
        }
        else
        {
            e2 = -M_PI/2;
            e1 = -e3 + delta;
        }
    }
    else
    {
        e2 = -asin(eerot[6]);
        IkReal cos_e2 = cos(e2);
        e1 = atan2(eerot[7]/cos_e2, eerot[8]/cos_e2);
        e3 = atan2(eerot[3]/cos_e2, eerot[0]/cos_e2);
   } 

    // radians to degrees
    world->a = e1*180.0/M_PI;
    world->b = e2*180.0/M_PI;
    world->c = e3*180.0/M_PI;

    world->u = 0; //joint[GetFreeParameters()[0]];


    //RTAPI_PRINT("ikfastkins::kinematicsForward(joints: " ); for ( i = 0; i < IKFAST_NUM_JOINTS; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(joint[i]) ); RTAPI_PRINT("\n");
    //RTAPI_PRINT("eerot: " ); for ( i = 0; i < 9; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(eerot[i]) ); RTAPI_PRINT("\n");
    //RTAPI_PRINT("eetrans: " ); for ( i = 0; i < 3; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(eetrans[i]) ); RTAPI_PRINT("\n");
	//RTAPI_PRINT("world: "DFMT" "DFMT" "DFMT" "DFMT" "DFMT" "DFMT" "DFMT" )\n", INTDOUBLE(world->tran.x), INTDOUBLE(world->tran.y), INTDOUBLE(world->tran.z), INTDOUBLE(world->a), INTDOUBLE(world->b), INTDOUBLE(world->c), INTDOUBLE(world->u) );
    
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

	// map free joints 
    jfree[0] = world->u / 1000.0; // mm to m
    jfree[0] += *(haldata->gripper_offset)/1000.0;

    {
        //eerot[0] = 1; eerot[1] = 0; eerot[2] = 0;
        //eerot[3] = 0; eerot[4] = 1; eerot[5] = 0;
        //eerot[6] = 0; eerot[7] = 0; eerot[8] = 1;
    }
    //memcpy( eerot, last_eerot, sizeof(last_eerot) );

    IkReal e1 = world->a / 180 * M_PI;
    IkReal e2 = world->b / 180 * M_PI;
    IkReal e3 = world->c / 180 * M_PI;
    IkReal cos_ps = cos(e1);
    IkReal sin_ps = sin(e1);
    IkReal cos_th = cos(e2);
    IkReal sin_th = sin(e2);
    IkReal cos_ph = cos(e3);
    IkReal sin_ph = sin(e3);
    eerot[0]=cos_th*cos_ph;
    eerot[1]=sin_ps*sin_th*cos_ph-cos_ps*sin_ph ;
    eerot[2]=cos_ps*sin_th*cos_ph+sin_ps*sin_ph;
    eerot[3]=cos_th*sin_ph;
    eerot[4]=sin_ps*sin_th*sin_ph+cos_ps*cos_ph ;
    eerot[5]=cos_ps*sin_th*sin_ph-sin_ps*cos_ph;
    eerot[6]=-sin_th;
    eerot[7]=sin_ps*cos_th;
    eerot[8]=cos_ps*cos_th;

    // convert linuxcnc mm to ikfast m
    eetrans[0] = world->tran.x/1000.0;
    eetrans[1] = world->tran.y/1000.0;
    eetrans[2] = world->tran.z/1000.0;


	//RTAPI_PRINT("ikfastkins::kinematicsInverse(world: "DFMT" "DFMT" "DFMT" "DFMT" "DFMT" "DFMT"\n", INTDOUBLE(world->tran.x), INTDOUBLE(world->tran.y), INTDOUBLE(world->tran.z), INTDOUBLE(world->a), INTDOUBLE(world->b), INTDOUBLE(world->c) );
    //RTAPI_PRINT("beforejoints: " ); for ( i = 0; i < IKFAST_NUM_JOINTS; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(joints[i]) ); RTAPI_PRINT("\n");
    //RTAPI_PRINT("eerot: " ); for ( i = 0; i < 9; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(eerot[i]) ); RTAPI_PRINT("\n");
    //RTAPI_PRINT("eetrans: " ); for ( i = 0; i < 3; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(eetrans[i]) ); RTAPI_PRINT("\n");

    long long tstart = rtapi_get_time();
    IkSolutionList_Init( &solutions );
    bSuccess = ComputeIk(eetrans, eerot, jfree, &solutions);
    long long tend = rtapi_get_time();
    double t = (tend-tstart)/1000.0;
    //RTAPI_PRINT("bSuccess=%d time="DFMT"us\n", bSuccess, INTDOUBLE(t));
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
        //RTAPI_PRINT("solns: %d - ", solutionCount );
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
                //RTAPI_PRINT("- joints: ");
                for( j = 0; j < solvalues_count; ++j)
                {
                    // convert from ikfast radians to linuxcnc degrees
                    solvalues[j] *= 180 / M_PI;
                    // Check joint ranges
                    if ( solvalues[j] < JointInfo[j].limitMin ||
                         solvalues[j] > JointInfo[j].limitMax )
                    {
                        outOfRange = true;
                        //RTAPI_PRINT("*" );
                    }
                    else
                    {
                        double diff = joints[j] - solvalues[j];
                        //while ( diff < -180 ) diff += 360;
                        //while ( diff > 180 ) diff -= 360;
                        err += diff*diff;
                    }
                    //RTAPI_PRINT(""DFMT" ", INTDOUBLE(solvalues[j]) );
                }
                if ( !outOfRange && err < bestSolutionDiff )
                {
                    bestSolution = s;
                    bestSolutionDiff = err;
                }
            }
            //RTAPI_PRINT(")\n" );
        }
        if ( bestSolution >= 0 && bestSolutionDiff <= MAX_SOLUTION_ERROR )
        {
            size_t j;
            IkSolution *sol = IkSolutionList_GetSolution(&solutions,bestSolution);
            IkReal solvalues[IKFAST_NUM_JOINTS];
            int solvalues_count;
            //RTAPI_PRINT( "bestSolutionDiff = "DFMT"\n", INTDOUBLE(bestSolutionDiff) );
            IkSolution_GetSolution(sol,solvalues, &solvalues_count, jfree);
            for( j = 0; j < solvalues_count; ++j)
            {
                joints[j] = solvalues[j] * 180 / M_PI;
            }
            //RTAPI_PRINT("afterjoints:  " ); for ( i = 0; i < IKFAST_NUM_JOINTS; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(joints[i]) ); RTAPI_PRINT("\n");
            no_result_count = 0;
            return GO_RESULT_OK;
        }
        else
        {
            if ( no_result_count == 0 )
            {
RTAPI_PRINT("ikfastkins::kinematicsInverse(world: "DFMT" "DFMT" "DFMT" "DFMT" "DFMT" "DFMT" "DFMT"\n", INTDOUBLE(world->tran.x), INTDOUBLE(world->tran.y), INTDOUBLE(world->tran.z), INTDOUBLE(world->a), INTDOUBLE(world->b), INTDOUBLE(world->c), INTDOUBLE(world->u) );
RTAPI_PRINT("beforejoints: " ); for ( i = 0; i < IKFAST_NUM_JOINTS; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(joints[i]) ); RTAPI_PRINT("\n");
RTAPI_PRINT("eerot: " ); for ( i = 0; i < 9; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(eerot[i]) ); RTAPI_PRINT("\n");
RTAPI_PRINT("eetrans: " ); for ( i = 0; i < 3; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(eetrans[i]) ); RTAPI_PRINT("\n");

        RTAPI_PRINT("solns: %d - ", solutionCount );
        {
            int s;
            for ( s = 0; s < solutionCount; s++ )
            {
                IkSolution *sol = IkSolutionList_GetSolution(&solutions,s);
                IkReal solvalues[IKFAST_NUM_JOINTS];
                size_t j;
                int solvalues_count;
                IkSolution_GetSolution(sol,solvalues,&solvalues_count,jfree);
                RTAPI_PRINT("\n- joints: ");
                for( j = 0; j < solvalues_count; ++j)
                {
                    // convert from ikfast radians to linuxcnc degrees
                    solvalues[j] *= 180 / M_PI;
                    // Check joint ranges
                    if ( solvalues[j] < JointInfo[j].limitMin ||
                         solvalues[j] > JointInfo[j].limitMax )
                    {
                        RTAPI_PRINT("*" );
                    }
                    RTAPI_PRINT(""DFMT" ", INTDOUBLE(solvalues[j]) );
                }
            }
            RTAPI_PRINT(")\n" );
        }
                 if ( bestSolution >= 0 )
                {
                    RTAPI_PRINT("ikfastkins: Unable to find close solution\n");
                    RTAPI_PRINT( "bestSolutionDiff = "DFMT"\n", INTDOUBLE(bestSolutionDiff) );
                }
                else
                    RTAPI_PRINT("ikfastkins: All solutions exceed joint limits\n");
            }
            no_result_count++;
            return GO_RESULT_ERROR;
        }
	}
    
	if ( no_result_count == 0 )
    {
RTAPI_PRINT("ikfastkins::kinematicsInverse(world: "DFMT" "DFMT" "DFMT" "DFMT" "DFMT" "DFMT" "DFMT"\n", INTDOUBLE(world->tran.x), INTDOUBLE(world->tran.y), INTDOUBLE(world->tran.z), INTDOUBLE(world->a), INTDOUBLE(world->b), INTDOUBLE(world->c), INTDOUBLE(world->u) );
RTAPI_PRINT("beforejoints: " ); for ( i = 0; i < IKFAST_NUM_JOINTS; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(joints[i]) ); RTAPI_PRINT("\n");
RTAPI_PRINT("eerot: " ); for ( i = 0; i < 9; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(eerot[i]) ); RTAPI_PRINT("\n");
RTAPI_PRINT("eetrans: " ); for ( i = 0; i < 3; i++ ) RTAPI_PRINT(""DFMT" ", INTDOUBLE(eetrans[i]) ); RTAPI_PRINT("\n");

        RTAPI_PRINT("ikfastkins: no solution\n");
    }
    no_result_count++;
    return GO_RESULT_ERROR;
}

int kinematicsHome(EmcPose * world,
    double *joint,
    KINEMATICS_FORWARD_FLAGS * fflags, KINEMATICS_INVERSE_FLAGS * iflags)
{
    /* use joints, set world */
	RTAPI_PRINT("ikfastkins::kinematicsHome\n");
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
    hal_pin_bit_newf( HAL_IN, &haldata->teleop_mode, comp_id, "%s.teleop-mode", COMPONENT_NAME );
    hal_pin_float_newf( HAL_OUT, &haldata->time_min, comp_id, "%s.time-min", COMPONENT_NAME );
    hal_pin_float_newf( HAL_OUT, &haldata->time_max, comp_id, "%s.time-max", COMPONENT_NAME );
    hal_pin_float_newf( HAL_OUT, &haldata->time_avg, comp_id, "%s.time-avg", COMPONENT_NAME );
    hal_pin_float_newf( HAL_OUT, &haldata->time_last, comp_id, "%s.time-last", COMPONENT_NAME );
    hal_pin_float_newf( HAL_IN, &haldata->gripper_offset, comp_id, "%s.gripper-offset", COMPONENT_NAME );
    *(haldata->teleop_mode) = false;
    *(haldata->time_min) = 0;
    *(haldata->time_max) = 0;
    *(haldata->time_avg) = 0;
    *(haldata->time_last) = 0;
    *(haldata->gripper_offset) = 0;
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

