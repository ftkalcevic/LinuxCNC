/********************************************************************
* Description: genserkins.c
*   Kinematics for a generalised serial kinematics machine
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

#include "rtapi_math.h"
#include "gotypes.h"		/* go_result, go_integer */
#include "gomath.h"		/* go_pose */
#include "posemath.h"
#include "kinematics.h"

#ifdef RTAPI
#include "rtapi.h"
#include "rtapi_app.h"
#endif

#include "hal.h"
#include "emcmotcfg.h"
//#include <string.h>

#define IKFAST_NO_MAIN
//#include "ikfast68.Translation3D.0_1_2_f3.c"
#include "ikfast68.TranslationZAxisAngle4D.0_1_2_3.c"
static IkReal last_eerot_0;

/* main function called by emc2 for forward Kins */
int kinematicsForward(const double *joint, 
                      EmcPose * world, 
                      const KINEMATICS_FORWARD_FLAGS * fflags, 
                      KINEMATICS_INVERSE_FLAGS * iflags) 
{
    IkReal j[EMCMOT_MAX_JOINTS], eetrans[EMCMOT_MAX_AXIS], eerot[9];
    int i;
	//rtapi_print("ikfastkins kinematicsForward\n");
    eerot[0] = 0; eerot[1] = 0; eerot[2] = 0;
    eerot[3] = 0; eerot[4] = 1; eerot[5] = 0;
    eerot[6] = 0; eerot[7] = 0; eerot[8] = 1;

    // Convert linuxcnc degrees to ikfast radians
    for ( i = 0; i < GetNumJoints(); i++ )
        j[i] = joint[i]/180.0*M_PI;

    ComputeFk(j, eetrans, eerot);

    last_eerot_0 = eerot[0];
    // Convert from ikfast m to linuxcnc mm
    world->tran.x = eetrans[0]*1000.0;
    world->tran.y = eetrans[1]*1000.0;
    world->tran.z = eetrans[2]*1000.0;

	//rtapi_print("ikfastkins::kinematicsForward(joints: %f %f %f %f %f %f - ", joint[0],joint[1],joint[2],joint[3],joint[4],joint[5]);
	//rtapi_print("eetrans: %f %f %f-%f %f %f-%f %f %f - ", eerot[0],eerot[1],eerot[2],eerot[3],eerot[4],eerot[5],eerot[6],eerot[7],eerot[8]);
	//rtapi_print("world: %f %f %f )\n", world->tran.x, world->tran.y, world->tran.x );
    
    return 0;
}


int kinematicsInverse(const EmcPose * world,
		      double *joints,
		      const KINEMATICS_INVERSE_FLAGS * iflags,
		      KINEMATICS_FORWARD_FLAGS * fflags)
{
    //return GO_RESULT_OK;
    IkReal jfree[EMCMOT_MAX_JOINTS], eerot[9], eetrans[3];
    int i;
    IkSolutionList solutions;
    bool bSuccess;

	//rtapi_print("ikfastkins kinematicsInverse\n");
    for ( i = 0; i < GetNumFreeParameters(); i++ )
    {
        jfree[i] = joints[GetFreeParameters()[i]]/180*M_PI;
    }
    eerot[0] = last_eerot_0; eerot[1] = 0; eerot[2] = 0;
    eerot[3] = 0; eerot[4] = 1; eerot[5] = 0;
    eerot[6] = 0; eerot[7] = 0; eerot[8] = 1;

    // convert linuxcnc mm to ikfast m
    eetrans[0] = world->tran.x/1000.0;
    eetrans[1] = world->tran.y/1000.0;
    eetrans[2] = world->tran.z/1000.0;

#define INTDOUBLE(x)    ((int)(x)), (((int)((x)*1000.0))%1000)
	rtapi_print("ikfastkins::kinematicsInverse(world: %d.%03d %d.%03d %d.%03d %d.%03d - ", INTDOUBLE(world->tran.x), INTDOUBLE(world->tran.y), INTDOUBLE(world->tran.z), INTDOUBLE(eerot[0]) );
    rtapi_print("beforejoints: %d.%03d %d.%03d %d.%03d %d.%03d %d.%03d %d.%03d\n", INTDOUBLE(joints[0]),INTDOUBLE(joints[1]),INTDOUBLE(joints[2]),INTDOUBLE(joints[3]),INTDOUBLE(joints[4]),INTDOUBLE(joints[5]));
    long long tstart = rtapi_get_time();
    IkSolutionList_Init( &solutions );
    bSuccess = ComputeIk(eetrans, eerot, jfree, &solutions);
    long long tend = rtapi_get_time();
    long t = (long)(tend-tstart)/1000;
    rtapi_print("bSuccess=%d time=%ld.%03ldms\n", bSuccess, t/1000, t%1000);

    if ( bSuccess )
    {
        int solutionCount = IkSolutionList_GetNumSolutions(&solutions);
        int bestSolution = 0;
        double bestSolutionDiff = 10000;
        //rtapi_print("solns: %d - ", solutionCount );
        if ( solutionCount > 1 )
        {
            int s;
            for ( s = 0; s < solutionCount; s++ )
            {
                IkSolution *sol = IkSolutionList_GetSolution(&solutions,s);
                IkReal vsolfree[IKFAST_NUM_DOF];
                IkReal solvalues[IKFAST_NUM_JOINTS];
                double err=0;
                size_t j;
                IkSolution_GetSolution(sol,solvalues,vsolfree);
                //rtapi_print("- joints: ");
                for( j = 0; j < IKFAST_NUM_DOF; ++j)
                {
                    // convert from ikfast radians to linuxcnc degrees
                    double angleDeg = solvalues[j] * 180 / M_PI;
                    double diff = joints[j] - angleDeg;
                    while ( diff < 180 ) diff += 360;
                    while ( diff > 180 ) diff -= 360;
                    err += diff*diff;
                    //rtapi_print("%f ", angleDeg );
                }
                if ( err < bestSolutionDiff )
                {
                    bestSolution = s;
                    bestSolutionDiff = err;
                }
            }
            //rtapi_print(")\n" );
        }
        {
            size_t j;
            IkSolution *sol = IkSolutionList_GetSolution(&solutions,bestSolution);
            IkReal vsolfree[IKFAST_NUM_DOF];
            IkReal solvalues[IKFAST_NUM_JOINTS];
            IkSolution_GetSolution(sol,solvalues,vsolfree);
            for( j = 0; j < IKFAST_NUM_JOINTS; ++j)
            {
                // convert from ikfast radians to linuxcnc degrees
                joints[j] = solvalues[j] * 180 / M_PI;
            }
        }
        rtapi_print("afterjoints: %d.%03d %d.%03d %d.%03d %d.%03d %d.%03d %d.%03d\n", INTDOUBLE(joints[0]),INTDOUBLE(joints[1]),INTDOUBLE(joints[2]),INTDOUBLE(joints[3]),INTDOUBLE(joints[4]),INTDOUBLE(joints[5]));
        return GO_RESULT_OK;
	}
    
	rtapi_print("no solution)\n");
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
	rtapi_print("Starting ikfastkins\n");
    comp_id = hal_init("ikfastkins");
    if (comp_id < 0)
	    return comp_id;

    hal_ready(comp_id);
	rtapi_print("ikfastkins ready\n");
    return 0;
}


void rtapi_app_exit(void)
{
    hal_exit(comp_id);
}
#endif

