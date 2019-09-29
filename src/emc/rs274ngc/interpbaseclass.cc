#include <assert.h>
#include <sys/time.h>
#include <stdarg.h>

#include "rs274ngc.hh"
#include "rs274ngc_interp.hh"
#include "interp_return.hh"
#include "canon.hh"
#include "config.h"		// LINELEN
#include "python_plugin.hh"
#include "interpbaseclass.hh"


#define RESULT_OK (result == INTERP_OK || result == INTERP_EXECUTE_FINISH)

// class Interp is the interpretter that parses the GCode.  It is a class, BUT, each action
// is implemnented as a global function, eg STRAIGHT_FEED() which makes it tricky to reuse.
static Interp *interp_new = NULL;

// To aid in reuse
static InterpBaseClass *pInterpBase = NULL;


int _task = 0; // control preview behaviour when remapping
char _parameter_file_name[LINELEN];

extern "C" void initinterpreter();
extern "C" void initemccanon();
extern "C" struct _inittab builtin_modules[];
struct _inittab builtin_modules[] = {
    //{ (char *) "interpreter", initinterpreter },
    //{ (char *) "emccanon", initemccanon },
    // any others...
    { NULL, NULL }
};


class UpdateCurPos 
{
    EmcPose &currentPos;
    EmcPose tmp_currentPos;
public:
    // Remember the new current pos
    UpdateCurPos( EmcPose &current_pos, double x, double y, double z, double a, double b, double c, double u, double v, double w )
        : currentPos( current_pos )
    {
        tmp_currentPos.tran.x=x; 
        tmp_currentPos.tran.y=y; 
        tmp_currentPos.tran.z=z; 
        tmp_currentPos.a=a; 
        tmp_currentPos.b=b; 
        tmp_currentPos.c=c;
        tmp_currentPos.u=u; 
        tmp_currentPos.v=v; 
        tmp_currentPos.w=w;
    }
    // and set it on descrution
    ~UpdateCurPos()
    {
        currentPos = tmp_currentPos;
    }
};

static bool check_abort() { return false; }

static void user_defined_function(int num, double arg1, double arg2) 
{
    pInterpBase->_UserDefinedFunction( num, arg1, arg2 );
}

void InterpBaseClass::_UserDefinedFunction(int num, double arg1, double arg2) 
{
    if ( interp_error ) 
        return;
    maybe_new_line( interp_new->sequence_number() );
    UserDefinedFunction( num, arg1, arg2 );
}

InterpBaseClass::InterpBaseClass( CANON_UNITS units )     // Units specify what units will returned by callbacks - CANON_UNITS_INCHES, CANON_UNITS_MM, CANON_UNITS_CM, or 0 internal units
{
    // There can only be one base instance.
    assert( pInterpBase == NULL );
    pInterpBase = this;
    
    // There can only be one interp_new.
    assert( interp_new == NULL );
    interp_new = new Interp();
    
    outputUnits = units;
    bScaleOutput=false;

    for ( int i = 0; i < USER_DEFINED_FUNCTION_NUM; i++ )    
    {
        USER_DEFINED_FUNCTION[i] = user_defined_function;
    }
    
}

//static void maybe_new_line(int sequence_number=interp_new->sequence_number());
void InterpBaseClass::maybe_new_line(int sequence_number) 
{
    // Callback for each line
    if ( interp_error ) 
        return;
    if ( sequence_number == last_sequence_number )
        return;

//    LineCode *new_line_code = (LineCode*)(PyObject_New(LineCode, &LineCodeType));
//    interp_new.active_settings(new_line_code->settings);
//    interp_new.active_g_codes(new_line_code->gcodes);
//    interp_new.active_m_codes(new_line_code->mcodes);
//    new_line_code->gcodes[0] = sequence_number;
    
    NextLine( sequence_number );
    last_sequence_number = sequence_number;
}

void InterpBaseClass::ScaleOutput( double &a1 ) const
{
    if ( bScaleOutput )
        a1 *= dOutputScale;
}

void InterpBaseClass::ScaleOutput( double &a1, double &a2 ) const
{
    ScaleOutput(a1);
    ScaleOutput(a2);
}

void InterpBaseClass::ScaleOutput( double &a1, double &a2, double &a3 ) const
{
    ScaleOutput(a1, a2);
    ScaleOutput(a3);
}

void InterpBaseClass::ScaleOutput( double &a1, double &a2, double &a3, double &a4, double &a5, double &a6 ) const
{
    ScaleOutput(a1,a2,a3);
    ScaleOutput(a4,a5,a6);
}

void InterpBaseClass::ScaleOutput( double &a1, double &a2, double &a3, double &a4, double &a5, double &a6, double &a7, double &a8 ) const
{
    ScaleOutput(a1,a2,a3,a4,a5,a6);
    ScaleOutput(a7,a8);
}

void InterpBaseClass::ScaleOutput( EmcPose &pos ) const
{
    ScaleOutput(pos.tran.x, pos.tran.y, pos.tran.z, pos.u, pos.v, pos.w );
}

void NURBS_FEED(int line_number, std::vector<CONTROL_POINT> nurbs_control_points, unsigned int k) 
{
    pInterpBase->_NurbsFeed( line_number, nurbs_control_points, k );
}

void InterpBaseClass::_NurbsFeed(int line_number, std::vector<CONTROL_POINT> nurbs_control_points, unsigned int k) 
{
    UpdateCurPos pos( current_pos, nurbs_control_points.back().X,nurbs_control_points.back().Y, current_pos.tran.z, current_pos.a, current_pos.b, current_pos.c, current_pos.u, current_pos.v, current_pos.w );

    if ( bScaleOutput ) 
    {
        for ( std::vector<CONTROL_POINT>::iterator it = nurbs_control_points.begin(); it != nurbs_control_points.end(); it++ )
        {
            ScaleOutput( it->X, it->Y ); 
        }
    }
    
    maybe_new_line(line_number);
    if (interp_error) 
        return;
    NurbsFeed(line_number, nurbs_control_points, k);
}

void ARC_FEED(int line_number,
              double first_end, double second_end, double first_axis,
              double second_axis, int rotation, double axis_end_point,
              double a_position, double b_position, double c_position,
              double u_position, double v_position, double w_position) 
{
    pInterpBase->_ArcFeed( line_number,
                          first_end, second_end, first_axis,
                          second_axis, rotation, axis_end_point,
                          a_position, b_position, c_position,
                          u_position, v_position, w_position );
}

void InterpBaseClass::_ArcFeed( int line_number,
                           double first_end, double second_end, double first_axis,
                           double second_axis, int rotation, double axis_end_point,
                           double a_position, double b_position, double c_position,
                           double u_position, double v_position, double w_position) 
{
    double x,y,z;
    switch ( plane )
    {
        case CANON_PLANE_XY:
            x = first_end;
            y = second_end;
            z = axis_end_point;
            break;
        case CANON_PLANE_YZ:
            y = first_end;
            z = second_end;
            x = axis_end_point;
            break;
        case CANON_PLANE_XZ:
            z = first_end;
            x = second_end;
            y = axis_end_point;
            break;
    }
    UpdateCurPos pos( current_pos, x, y, z, current_pos.a, current_pos.b, current_pos.c, current_pos.u, current_pos.v, current_pos.w );

    ScaleOutput( first_end, second_end, first_axis, second_axis, axis_end_point, u_position, v_position, w_position );

    maybe_new_line(line_number);
    if (interp_error) 
        return;
    ArcFeed( line_number,
             first_end, second_end, first_axis,
             second_axis, rotation, axis_end_point,
             a_position, b_position, c_position,
             u_position, v_position, w_position );
}

void STRAIGHT_FEED( int line_number,
                    double x, double y, double z,
                    double a, double b, double c,
                    double u, double v, double w ) 
{
    pInterpBase->_StraightFeed( line_number, x, y, z, a, b, c, u, v, w  );
}

void InterpBaseClass::_StraightFeed( int line_number,
                                double x, double y, double z,
                                double a, double b, double c,
                                double u, double v, double w ) 
{
    UpdateCurPos pos( current_pos, x, y, z, a, b, c, u, v, w );
    
    ScaleOutput( x, y, z, u, v, w );
    
    maybe_new_line(line_number);
    if (interp_error) 
        return;
    StraightFeed( line_number, x, y, z, a, b, c, u, v, w );
}

void STRAIGHT_TRAVERSE(int line_number,
                       double x, double y, double z,
                       double a, double b, double c,
                       double u, double v, double w) 
{
    pInterpBase->_StraightTraverse( line_number,
                                    x, y, z,
                                    a, b, c,
                                    u, v, w );
}


void InterpBaseClass::_StraightTraverse( int line_number,
                                   double x, double y, double z,
                                   double a, double b, double c,
                                   double u, double v, double w ) 
{
    UpdateCurPos pos( current_pos, x, y, z, a, b, c, u, v, w );

    ScaleOutput( x, y, z, u, v, w );
    
    maybe_new_line(line_number);
    if ( interp_error ) 
        return;
    StraightTraverse( line_number,
                      x, y, z,
                      a, b, c,
                      u, v, w );
}

void SET_G5X_OFFSET(int g5x_index,
                    double x, double y, double z,
                    double a, double b, double c,
                    double u, double v, double w) 
{
    pInterpBase->_SetG5XOffset( g5x_index,
                                x, y, z,
                                a, b, c,
                                u, v, w );
}

void InterpBaseClass::_SetG5XOffset( int index,
                                double x, double y, double z,
                                double a, double b, double c,
                                double u, double v, double w ) 
{
    EmcPose g5x = { {x, y, z}, a, b, c, u, v, w };
    g5x_offset = g5x;
    g5x_index = index;
    
    ScaleOutput( x, y, z, u, v, w );
    
    maybe_new_line( interp_new->sequence_number() );
    if ( interp_error ) 
        return;
    SetG5XOffset( index,
                  x, y, z,
                  a, b, c,
                  u, v, w );
}

void SET_G92_OFFSET(double x, double y, double z,
                    double a, double b, double c,
                    double u, double v, double w) 
{
    pInterpBase->_SetG92Offset( x, y, z,
                                a, b, c,
                                u, v, w);
}

void InterpBaseClass::_SetG92Offset( double x, double y, double z,
                                double a, double b, double c,
                                double u, double v, double w )
{
    g92_offset.tran.x=x;
    g92_offset.tran.y=y;
    g92_offset.tran.z=z; 
    g92_offset.a=a;     
    g92_offset.b=b;     
    g92_offset.c=c;     
    g92_offset.u=u;     
    g92_offset.v=v;     
    g92_offset.w=w;     

    ScaleOutput( x, y, z, u, v, w );
    
    maybe_new_line( interp_new->sequence_number() );
    if ( interp_error ) 
        return;
    SetG92Offset( x, y, z,
                  a, b, c,
                  u, v, w );
}

void SET_XY_ROTATION(double t) 
{
    pInterpBase->_SetXYRotation(t);
}
void InterpBaseClass::_SetXYRotation(double t) 
{
    xy_rotation = t;
    maybe_new_line( interp_new->sequence_number() );
    if (interp_error ) 
        return;
    SetXYRotation(t);
}

void USE_LENGTH_UNITS(CANON_UNITS u) 
{ 
    pInterpBase->_UseLengthUnits(u) ;
}

void InterpBaseClass::_UseLengthUnits(CANON_UNITS u) 
{ 
//    if ( outputUnits == 0 ||    // Don't care
//         outputUnits == u )      // units match what we want
//    {
        bScaleOutput = false;
//    }
//    else
//    {
//        bScaleOutput = true;
//        dOutputScale = 1.0;
//        switch ( outputUnits )
//        {
//            case CANON_UNITS_MM:
//                switch ( u )
//                {
//                    case CANON_UNITS_CM:        dOutputScale = 10.0; break;
//                    case CANON_UNITS_INCHES:    dOutputScale = 25.4; break;
//                };
//                break;
//            case CANON_UNITS_CM:
//                switch ( u )
//                {
//                    case CANON_UNITS_MM:        dOutputScale = 1.0/10.0; break;
//                    case CANON_UNITS_INCHES:    dOutputScale = 2.54; break;
//                };
//                break;
//            case CANON_UNITS_INCHES:
//                switch ( u )
//                {
//                    case CANON_UNITS_MM:        dOutputScale = 1.0/25.4; break;
//                    case CANON_UNITS_CM:        dOutputScale = 1.0/2.54; break;
//                };
//                break;
//        }
//    }
    UseLengthUnits(u);
}

void SELECT_PLANE(CANON_PLANE pl) 
{
    pInterpBase->_SelectPlane(pl);
}

void InterpBaseClass::_SelectPlane(CANON_PLANE pl) 
{
    plane = pl;
    maybe_new_line( interp_new->sequence_number() );
    if ( interp_error ) 
        return;
    SelectPlane(pl);
}

void SET_TRAVERSE_RATE(double rate) 
{
    pInterpBase->_SetTraverseRate(rate);
}

void InterpBaseClass::_SetTraverseRate(double rate) 
{
    maybe_new_line( interp_new->sequence_number() );
    if ( interp_error ) 
        return;
    SetTraverseRate(rate);
}

void SET_FEED_MODE(int mode) 
{
    pInterpBase->_SetFeedMode(mode);
}

void InterpBaseClass::_SetFeedMode(int mode) 
{
    maybe_new_line( interp_new->sequence_number() );
    if (interp_error) 
        return;
    SetFeedMode(mode);
}

void CHANGE_TOOL(int pocket) 
{
    pInterpBase->_ChangeTool(pocket);
}

void InterpBaseClass::_ChangeTool(int pocket) 
{
    maybe_new_line( interp_new->sequence_number() );
    if(interp_error) 
        return;
    ChangeTool(pocket);
}

void CHANGE_TOOL_NUMBER(int pocket) 
{
    pInterpBase->_ChangeToolNumber(pocket);
}

void InterpBaseClass::_ChangeToolNumber(int pocket) 
{
    maybe_new_line( interp_new->sequence_number() );
    if (interp_error) 
        return;
    ChangeToolNumber(pocket);
}

/* XXX: This needs to be re-thought.  Sometimes feed rate is not in linear
 * units--e.g., it could be inverse time feed mode.  in that case, it's wrong
 * to convert from mm to inch here.  but the gcode time estimate gets inverse
 * time feed wrong anyway..
 */
void SET_FEED_RATE(double rate) 
{
    pInterpBase->_SetFeedRate(rate);
}

void InterpBaseClass::_SetFeedRate(double rate) 
{
    maybe_new_line( interp_new->sequence_number() );
    if (interp_error) 
        return;
    ScaleOutput( rate );

    SetFeedRate(rate);
}

void DWELL(double time) 
{
    pInterpBase->_Dwell(time);
}

void InterpBaseClass::_Dwell(double time) 
{
    maybe_new_line( interp_new->sequence_number() );
    if(interp_error) 
        return;
    Dwell(time);
}

void MESSAGE(char *comment) 
{
    pInterpBase->_Message(comment);
}

void InterpBaseClass::_Message(char *comment) 
{
    maybe_new_line( interp_new->sequence_number() );
    if(interp_error) 
        return;
    Message(comment);
}

void LOG(char *s) 
{
    pInterpBase->Log(s);
}

void LOGOPEN(char *f) 
{
    pInterpBase->LogOpen(f);
}

void LOGAPPEND(char *f) 
{
    pInterpBase->LogAppend(f);
}

void LOGCLOSE() 
{
    pInterpBase->LogClose();
}

void COMMENT(const char *comment) 
{
    pInterpBase->_Comment(comment);
}

void InterpBaseClass::_Comment(const char *comment) 
{
    maybe_new_line( interp_new->sequence_number() );
    if (interp_error) 
        return;
    Comment(comment);
}

void SET_TOOL_TABLE_ENTRY(int pocket, int toolno, EmcPose offset, double diameter,
                          double frontangle, double backangle, int orientation) {
    pInterpBase->_SetToolTableEntry( pocket, toolno, offset, diameter,
                                     frontangle, backangle, orientation);
}

void InterpBaseClass::_SetToolTableEntry(int pocket, int toolno, EmcPose offset, double diameter,
                                    double frontangle, double backangle, int orientation) 
{
    maybe_new_line( interp_new->sequence_number() );
    if (interp_error) 
        return;
    SetToolTableEntry( pocket, toolno, offset, diameter,
                       frontangle, backangle, orientation);
}

void USE_TOOL_LENGTH_OFFSET(EmcPose offset) 
{
    pInterpBase->_UseToolLengthOffset(offset);
}

void InterpBaseClass::_UseToolLengthOffset(EmcPose offset) 
{
    tool_offset = offset;
    maybe_new_line( interp_new->sequence_number() );
    if(interp_error) 
        return;
    ScaleOutput( offset );

    UseToolLengthOffset(offset);
}

void SET_FEED_REFERENCE(double reference) 
{ 
    pInterpBase->SetFeedReference(reference);
}

void SET_CUTTER_RADIUS_COMPENSATION(double radius) 
{
    pInterpBase->SetCutterRadiusCompensation(radius);
}

void START_CUTTER_RADIUS_COMPENSATION(int direction) 
{
    pInterpBase->StartCutterRadiusCompensation(direction);
}

void STOP_CUTTER_RADIUS_COMPENSATION(int direction) 
{
    pInterpBase->StopCutterRadiusCompensation(direction);
}
void START_SPEED_FEED_SYNCH() 
{
    pInterpBase->StartSpeedFeedSynch();
}

void START_SPEED_FEED_SYNCH(double sync, bool vel) 
{
    pInterpBase->StartSpeedFeedSynch(sync, vel);
}

void STOP_SPEED_FEED_SYNCH() 
{
    pInterpBase->StopSpeedFeedSynch();
}

void START_SPINDLE_COUNTERCLOCKWISE() 
{
    pInterpBase->StartSpindleCounterclockwise();
}

void START_SPINDLE_CLOCKWISE() 
{
    pInterpBase->StartSpindleClockwise();
}
void SET_SPINDLE_MODE(double mode) 
{
    pInterpBase->SetSpindleMode(mode);
}
void STOP_SPINDLE_TURNING() 
{
    pInterpBase->StopSpindleTurning();
}
void SET_SPINDLE_SPEED(double rpm) 
{
    pInterpBase->SetSpindleSpeed(rpm);
}
void ORIENT_SPINDLE(double d, int i) 
{
    pInterpBase->OrientSpindle(d, i);
}
void WAIT_SPINDLE_ORIENT_COMPLETE(double timeout) 
{
    pInterpBase->WaitSpindleOrientComplete(timeout) ;
}
void PROGRAM_STOP() 
{
    pInterpBase->ProgramStop();
}
void PROGRAM_END() 
{
    pInterpBase->ProgramEnd();
}
void FINISH() 
{
    pInterpBase->Finish();
}
void PALLET_SHUTTLE() 
{
    pInterpBase->PalletShuttle();
}
void SELECT_POCKET(int pocket, int tool) 
{
    pInterpBase->SelectPocket(pocket, tool);
}
void OPTIONAL_PROGRAM_STOP() 
{
    pInterpBase->OptionalProgramStop();
}
void START_CHANGE() 
{
    pInterpBase->StartChange();
}
int  GET_EXTERNAL_TC_FAULT()
{
    return pInterpBase->GetExternalTCFault();
}

int  GET_EXTERNAL_TC_REASON()
{
    return pInterpBase->GetExternalTCReason();
}

bool GET_BLOCK_DELETE(void) { 
    return pInterpBase->GetBlockDelete();
}

void CANON_ERROR(const char *fmt, ...) 
{
    va_list args;
    va_start(args,fmt);
    pInterpBase->CanonError( fmt, args );
}

void CLAMP_AXIS(CANON_AXIS axis) 
{
    pInterpBase->ClampAxis(axis);
}

bool GET_OPTIONAL_PROGRAM_STOP() 
{ 
    return pInterpBase->GetOptionalProgramStop();
}

void SET_OPTIONAL_PROGRAM_STOP(bool state) 
{
    pInterpBase->setOptionalProgramStop(state);
}
void SPINDLE_RETRACT_TRAVERSE()
{
    pInterpBase->SpindlRetractTraverse();
}

void SPINDLE_RETRACT()
{
    pInterpBase->SpindleRetract();
}

void STOP_CUTTER_RADIUS_COMPENSATION()
{
    pInterpBase->StopCutterRadiusCompensation();
}

void USE_NO_SPINDLE_FORCE()
{
    pInterpBase->UseNoSpindleForce();
}

void SET_BLOCK_DELETE(bool enabled)
{
    pInterpBase->SetBlockDelete(enabled);
}


void DISABLE_FEED_OVERRIDE()
{
    pInterpBase->DisableFeedOverride();
}

void DISABLE_FEED_HOLD()
{
    pInterpBase->DisableFeedHold();
}

void ENABLE_FEED_HOLD()
{
    pInterpBase->EnableFeedHold();
}

void DISABLE_SPEED_OVERRIDE() 
{
    pInterpBase->DisableSpeedOverride();
}

void ENABLE_FEED_OVERRIDE()
{
    pInterpBase->EnableFeedOverride();
}

void ENABLE_SPEED_OVERRIDE()
{
    pInterpBase->EnableSpeedOverride();
}

void MIST_OFF()
{
    pInterpBase->MistOff();
}

void FLOOD_OFF()
{
    pInterpBase->FloodOff();
}

void MIST_ON()
{
    pInterpBase->MistOn();
}

void FLOOD_ON() 
{
    pInterpBase->FloodOn();
}

void CLEAR_AUX_OUTPUT_BIT(int bit)
{
    pInterpBase->ClearAuxOutputBit( bit );
}

void SET_AUX_OUTPUT_BIT(int bit)
{
    pInterpBase->SetAuxOutputBit(bit);
}

void SET_AUX_OUTPUT_VALUE(int index, double value)
{
    pInterpBase->SetAuxOutputValue(index, value);
}

void CLEAR_MOTION_OUTPUT_BIT(int bit) 
{
    pInterpBase->ClearMotionOutputBit(bit);
}

void SET_MOTION_OUTPUT_BIT(int bit)
{
    pInterpBase->SetMotionOutputBit(bit);
}

void SET_MOTION_OUTPUT_VALUE(int index, double value)
{
    pInterpBase->SetMotionOutputValue(index, value);
}

void TURN_PROBE_ON() 
{
    pInterpBase->TurnProbeOn();
}

void TURN_PROBE_OFF() 
{
    pInterpBase->TurnProbeOff();
}

int UNLOCK_ROTARY(int line_no, int axis)
{
    return pInterpBase->UnlockRotary(line_no, axis);
}

int LOCK_ROTARY(int line_no, int axis)
{
    return pInterpBase->LockRotary(line_no, axis);
}

void INTERP_ABORT(int reason,const char *message)
{
    pInterpBase->InterpAbort( reason, message );
}

void PLUGIN_CALL(int len, const char *call)
{
    pInterpBase->PluginCall( len, call );
}

void IO_PLUGIN_CALL(int len, const char *call)
{
    pInterpBase->IOPluginCall( len, call );
}


void STRAIGHT_PROBE(int line_number, 
                    double x, double y, double z, 
                    double a, double b, double c,
                    double u, double v, double w, unsigned char probe_type) 
{
    pInterpBase->_StraightProbe( line_number, 
                                 x, y, z, 
                                 a, b, c,
                                 u, v, w, probe_type);
}

void InterpBaseClass::_StraightProbe( int line_number, 
                                 double x, double y, double z, 
                                 double a, double b, double c,
                                 double u, double v, double w, unsigned char probe_type ) 
{
    UpdateCurPos pos( current_pos, x, y, z, a, b, c, u, v, w );

    ScaleOutput( x, y, z, u, v, w );

    maybe_new_line(line_number);
    if(interp_error) 
        return;
    StraightProbe( line_number, 
                   x, y, z, 
                   a, b, c,
                   u, v, w, probe_type);
}


void RIGID_TAP(int line_number,
               double x, double y, double z) 
{
    pInterpBase->_RigidTap( line_number,
                            x, y, z);
}
void InterpBaseClass::_RigidTap( int line_number,
                            double x, double y, double z ) 
{
    ScaleOutput( x, y, z );
    
    maybe_new_line(line_number);
    RigidTap( line_number,
              x, y, z );
}

double GET_EXTERNAL_MOTION_CONTROL_TOLERANCE() { return 0.1; }
double GET_EXTERNAL_PROBE_POSITION_X() { return pInterpBase->CurrentPosRaw().tran.x; }
double GET_EXTERNAL_PROBE_POSITION_Y() { return pInterpBase->CurrentPosRaw().tran.y; }
double GET_EXTERNAL_PROBE_POSITION_Z() { return pInterpBase->CurrentPosRaw().tran.z; }
double GET_EXTERNAL_PROBE_POSITION_A() { return pInterpBase->CurrentPosRaw().a; }
double GET_EXTERNAL_PROBE_POSITION_B() { return pInterpBase->CurrentPosRaw().b; }
double GET_EXTERNAL_PROBE_POSITION_C() { return pInterpBase->CurrentPosRaw().c; }
double GET_EXTERNAL_PROBE_POSITION_U() { return pInterpBase->CurrentPosRaw().u; }
double GET_EXTERNAL_PROBE_POSITION_V() { return pInterpBase->CurrentPosRaw().v; }
double GET_EXTERNAL_PROBE_POSITION_W() { return pInterpBase->CurrentPosRaw().w; }
double GET_EXTERNAL_PROBE_VALUE() { return 0.0; }
int GET_EXTERNAL_PROBE_TRIPPED_VALUE() { return 0; }
double GET_EXTERNAL_POSITION_X() { return pInterpBase->CurrentPosRaw().tran.x; }
double GET_EXTERNAL_POSITION_Y() { return pInterpBase->CurrentPosRaw().tran.y; }
double GET_EXTERNAL_POSITION_Z() { return pInterpBase->CurrentPosRaw().tran.z; }
double GET_EXTERNAL_POSITION_A() { return pInterpBase->CurrentPosRaw().a; }
double GET_EXTERNAL_POSITION_B() { return pInterpBase->CurrentPosRaw().b; }
double GET_EXTERNAL_POSITION_C() { return pInterpBase->CurrentPosRaw().c; }
double GET_EXTERNAL_POSITION_U() { return pInterpBase->CurrentPosRaw().u; }
double GET_EXTERNAL_POSITION_V() { return pInterpBase->CurrentPosRaw().v; }
double GET_EXTERNAL_POSITION_W() { return pInterpBase->CurrentPosRaw().w; }
void INIT_CANON() 
{
    pInterpBase->InitCanon();
}

void GET_EXTERNAL_PARAMETER_FILE_NAME(char *name, int max_size) 
{
    if ( max_size > 0 )
        name[0] = 0;
    pInterpBase->GetExternalParameterFileName(name, max_size);
}
CANON_UNITS GET_EXTERNAL_LENGTH_UNIT_TYPE() 
{ 
    return pInterpBase->GetExternalLengthUnitType();
}
CANON_TOOL_TABLE GET_EXTERNAL_TOOL_TABLE(int pocket) 
{
    return pInterpBase->GetExternalToolTable(pocket);
}

int GET_EXTERNAL_DIGITAL_INPUT(int index, int def) 
{ 
    return pInterpBase->GetExternalDigitalInput( index, def);
}

double GET_EXTERNAL_ANALOG_INPUT(int index, double def) 
{ 
    return pInterpBase->GetExternalAnalogInput( index, def ) ;
}
int WAIT(int index, int input_type, int wait_type, double timeout) 
{ 
    return pInterpBase->Wait( index, input_type, wait_type, timeout);
}

void SET_FEED_REFERENCE(int ref) { pInterpBase->SetFeedReference( ref ); }
int GET_EXTERNAL_QUEUE_EMPTY() { return pInterpBase->GetExternalQueueEmpty();}
CANON_DIRECTION GET_EXTERNAL_SPINDLE() { return pInterpBase->GetExternalSpindle(); }
int GET_EXTERNAL_TOOL_SLOT() { return pInterpBase->GetExternalToolSlot(); }
int GET_EXTERNAL_SELECTED_TOOL_SLOT() { return pInterpBase->GetExternalSelectedToolSlot(); }
double GET_EXTERNAL_FEED_RATE() { return pInterpBase->GetExternalFeedRate(); }
double GET_EXTERNAL_TRAVERSE_RATE() { return pInterpBase->GetExternalTraverseRate(); }
int GET_EXTERNAL_FLOOD() { return pInterpBase->GetExternalFlood(); }
int GET_EXTERNAL_MIST() { return pInterpBase->GetExternalMist(); }
CANON_PLANE GET_EXTERNAL_PLANE() { return pInterpBase->GetExternalPlane(); }
double GET_EXTERNAL_SPEED() { return pInterpBase->GetExternalSpeed(); }
int GET_EXTERNAL_POCKETS_MAX() { return pInterpBase->GetExternalPocketsMax(); }
void DISABLE_ADAPTIVE_FEED() {pInterpBase->DisableAdaptiveFeed(); } 
void ENABLE_ADAPTIVE_FEED() { pInterpBase->EnableAdaptiveFeed(); } 

int GET_EXTERNAL_FEED_OVERRIDE_ENABLE() {return pInterpBase->GetExternalFeedOverrideEnable();}
int GET_EXTERNAL_SPINDLE_OVERRIDE_ENABLE() {return pInterpBase->GetExternalSpindleOverrideEnable(); }
int GET_EXTERNAL_ADAPTIVE_FEED_ENABLE() {return pInterpBase->GetExternalAdaptiveFeedEnable();}
int GET_EXTERNAL_FEED_HOLD_ENABLE() {return pInterpBase->GetExternalFeedHoldEnable();}

int GET_EXTERNAL_AXIS_MASK() 
{
    return pInterpBase->GetExternalAxisMask();
}

double GET_EXTERNAL_TOOL_LENGTH_XOFFSET() {
    return pInterpBase->ToolOffsetRaw().tran.x;
}
double GET_EXTERNAL_TOOL_LENGTH_YOFFSET() {
    return pInterpBase->ToolOffsetRaw().tran.y;
}
double GET_EXTERNAL_TOOL_LENGTH_ZOFFSET() {
    return pInterpBase->ToolOffsetRaw().tran.z;
}
double GET_EXTERNAL_TOOL_LENGTH_AOFFSET() {
    return pInterpBase->ToolOffsetRaw().a;
}
double GET_EXTERNAL_TOOL_LENGTH_BOFFSET() {
    return pInterpBase->ToolOffsetRaw().b;
}
double GET_EXTERNAL_TOOL_LENGTH_COFFSET() {
    return pInterpBase->ToolOffsetRaw().c;
}
double GET_EXTERNAL_TOOL_LENGTH_UOFFSET() {
    return pInterpBase->ToolOffsetRaw().u;
}
double GET_EXTERNAL_TOOL_LENGTH_VOFFSET() {
    return pInterpBase->ToolOffsetRaw().v;
}
double GET_EXTERNAL_TOOL_LENGTH_WOFFSET() {
    return pInterpBase->ToolOffsetRaw().w;
}

double GET_EXTERNAL_ANGLE_UNITS() {
    return pInterpBase->GetExternalAngleUnits();
}

double GET_EXTERNAL_LENGTH_UNITS() {
    return pInterpBase->GetExternalLengthUnits();
}

// User defined M codes 100-199
USER_DEFINED_FUNCTION_TYPE USER_DEFINED_FUNCTION[USER_DEFINED_FUNCTION_NUM];

CANON_MOTION_MODE motion_mode;
void SET_MOTION_CONTROL_MODE(CANON_MOTION_MODE mode, double tolerance) { motion_mode = mode; }
void SET_MOTION_CONTROL_MODE(double tolerance) { }
void SET_MOTION_CONTROL_MODE(CANON_MOTION_MODE mode) { motion_mode = mode; }
CANON_MOTION_MODE GET_EXTERNAL_MOTION_CONTROL_MODE() { return motion_mode; }
void SET_NAIVECAM_TOLERANCE(double tolerance) { }

void InterpBaseClass::parse_file( const char *filename )
{
    char *unitcode=0, *initcode=0;
    //int error_line_offset = 0;
    struct timeval t0, t1;
    int wait = 1;

    gettimeofday(&t0, NULL);

    interp_error = 0;
    last_sequence_number = -1;
    ZERO_EMC_POSE(current_pos);
    ZERO_EMC_POSE(g92_offset);
    ZERO_EMC_POSE(tool_offset);
    ZERO_EMC_POSE(g5x_offset);
    g5x_index = 1;
    xy_rotation = 0;
    plane = CANON_PLANE_XY;

    interp_new->init();
    interp_new->open(filename);

    maybe_new_line( interp_new->sequence_number() );

    int result = INTERP_OK;
    if(unitcode) 
    {
        result = interp_new->read(unitcode);
        if(!RESULT_OK) goto out_error;
        result = interp_new->execute();
    }
    if(initcode && RESULT_OK) 
    {
        result = interp_new->read(initcode);
        if(!RESULT_OK) goto out_error;
        result = interp_new->execute();
    }
    while(!interp_error && RESULT_OK) 
    {
        //error_line_offset = 1;
        result = interp_new->read();
        gettimeofday(&t1, NULL);
        if(t1.tv_sec > t0.tv_sec + wait) 
        {
            if(check_abort()) return ;
            t0 = t1;
        }
        if(!RESULT_OK) break;
        //error_line_offset = 0;
        result = interp_new->execute();
    }

out_error:
    // last line = last_sequence_number
    interp_new->close();
    if(interp_error || result > INTERP_MIN_ERROR ) 
    {
        // TODO - better placing
        CANON_ERROR( "Error processing gcode: Line %d: %s", interp_new->sequence_number(), interp_new->getSavedError() ) ;
        return ;
    }
    maybe_new_line( interp_new->sequence_number() );
}

/*
 
static int maxerror = -1;

static char savedError[LINELEN+1];
static PyObject *rs274_strerror(PyObject *s, PyObject *o) {
    int err;
    if(!PyArg_ParseTuple(o, "i", &err)) return NULL;
    interp_new.error_text(err, savedError, LINELEN);
    return PyString_FromString(savedError);
}

static PyObject *rs274_calc_extents(PyObject *self, PyObject *args) {
    double min_x = 9e99, min_y = 9e99, min_z = 9e99,
           min_xt = 9e99, min_yt = 9e99, min_zt = 9e99,
           max_x = -9e99, max_y = -9e99, max_z = -9e99,
           max_xt = -9e99, max_yt = -9e99, max_zt = -9e99;
    for(int i=0; i<PySequence_Length(args); i++) {
        PyObject *si = PyTuple_GetItem(args, i);
        if(!si) return NULL;
        int j;
        double xs, ys, zs, xe, ye, ze, xt, yt, zt;
        for(j=0; j<PySequence_Length(si); j++) {
            PyObject *sj = PySequence_GetItem(si, j);
            PyObject *unused;
            int r;
            if(PyTuple_Size(sj) == 4)
                r = PyArg_ParseTuple(sj,
                    "O(dddOOOOOO)(dddOOOOOO)(ddd):calc_extents item",
                    &unused,
                    &xs, &ys, &zs, &unused, &unused, &unused, &unused, &unused, &unused,
                    &xe, &ye, &ze, &unused, &unused, &unused, &unused, &unused, &unused,
                    &xt, &yt, &zt);
            else
                r = PyArg_ParseTuple(sj,
                    "O(dddOOOOOO)(dddOOOOOO)O(ddd):calc_extents item",
                    &unused,
                    &xs, &ys, &zs, &unused, &unused, &unused, &unused, &unused, &unused,
                    &xe, &ye, &ze, &unused, &unused, &unused, &unused, &unused, &unused,
                    &unused, &xt, &yt, &zt);
            Py_DECREF(sj);
            if(!r) return NULL;
            max_x = std::max(max_x, xs);
            max_y = std::max(max_y, ys);
            max_z = std::max(max_z, zs);
            min_x = std::min(min_x, xs);
            min_y = std::min(min_y, ys);
            min_z = std::min(min_z, zs);
            max_xt = std::max(max_xt, xs+xt);
            max_yt = std::max(max_yt, ys+yt);
            max_zt = std::max(max_zt, zs+zt);
            min_xt = std::min(min_xt, xs+xt);
            min_yt = std::min(min_yt, ys+yt);
            min_zt = std::min(min_zt, zs+zt);
        }
        if(j > 0) {
            max_x = std::max(max_x, xe);
            max_y = std::max(max_y, ye);
            max_z = std::max(max_z, ze);
            min_x = std::min(min_x, xe);
            min_y = std::min(min_y, ye);
            min_z = std::min(min_z, ze);
            max_xt = std::max(max_xt, xe+xt);
            max_yt = std::max(max_yt, ye+yt);
            max_zt = std::max(max_zt, ze+zt);
            min_xt = std::min(min_xt, xe+xt);
            min_yt = std::min(min_yt, ye+yt);
            min_zt = std::min(min_zt, ze+zt);
        }
    }
    return Py_BuildValue("[ddd][ddd][ddd][ddd]",
        min_x, min_y, min_z,  max_x, max_y, max_z,
        min_xt, min_yt, min_zt,  max_xt, max_yt, max_zt);
}


static void unrotate(double &x, double &y, double c, double s) {
    double tx = x * c + y * s;
    y = -x * s + y * c;
    x = tx;
}

static void rotate(double &x, double &y, double c, double s) {
    double tx = x * c - y * s;
    y = x * s + y * c;
    x = tx;
}

static PyObject *rs274_arc_to_segments(PyObject *self, PyObject *args) {
    PyObject *canon;
    double x1, y1, cx, cy, z1, a, b, c, u, v, w;
    double o[9], n[9], g5xoffset[9], g92offset[9];
    int rot, plane;
    int X, Y, Z;
    double rotation_cos, rotation_sin;
    int max_segments = 128;

    if(!PyArg_ParseTuple(args, "Oddddiddddddd|i:arcs_to_segments",
        &canon, &x1, &y1, &cx, &cy, &rot, &z1, &a, &b, &c, &u, &v, &w, &max_segments)) return NULL;
    if(!get_attr(canon, "lo", "ddddddddd:arcs_to_segments lo", &o[0], &o[1], &o[2],
                    &o[3], &o[4], &o[5], &o[6], &o[7], &o[8]))
        return NULL;
    if(!get_attr(canon, "plane", &plane)) return NULL;
    if(!get_attr(canon, "rotation_cos", &rotation_cos)) return NULL;
    if(!get_attr(canon, "rotation_sin", &rotation_sin)) return NULL;
    if(!get_attr(canon, "g5x_offset_x", &g5xoffset[0])) return NULL;
    if(!get_attr(canon, "g5x_offset_y", &g5xoffset[1])) return NULL;
    if(!get_attr(canon, "g5x_offset_z", &g5xoffset[2])) return NULL;
    if(!get_attr(canon, "g5x_offset_a", &g5xoffset[3])) return NULL;
    if(!get_attr(canon, "g5x_offset_b", &g5xoffset[4])) return NULL;
    if(!get_attr(canon, "g5x_offset_c", &g5xoffset[5])) return NULL;
    if(!get_attr(canon, "g5x_offset_u", &g5xoffset[6])) return NULL;
    if(!get_attr(canon, "g5x_offset_v", &g5xoffset[7])) return NULL;
    if(!get_attr(canon, "g5x_offset_w", &g5xoffset[8])) return NULL;
    if(!get_attr(canon, "g92_offset_x", &g92offset[0])) return NULL;
    if(!get_attr(canon, "g92_offset_y", &g92offset[1])) return NULL;
    if(!get_attr(canon, "g92_offset_z", &g92offset[2])) return NULL;
    if(!get_attr(canon, "g92_offset_a", &g92offset[3])) return NULL;
    if(!get_attr(canon, "g92_offset_b", &g92offset[4])) return NULL;
    if(!get_attr(canon, "g92_offset_c", &g92offset[5])) return NULL;
    if(!get_attr(canon, "g92_offset_u", &g92offset[6])) return NULL;
    if(!get_attr(canon, "g92_offset_v", &g92offset[7])) return NULL;
    if(!get_attr(canon, "g92_offset_w", &g92offset[8])) return NULL;

    if(plane == 1) {
        X=0; Y=1; Z=2;
    } else if(plane == 3) {
        X=2; Y=0; Z=1;
    } else {
        X=1; Y=2; Z=0;
    }
    n[X] = x1;
    n[Y] = y1;
    n[Z] = z1;
    n[3] = a;
    n[4] = b;
    n[5] = c;
    n[6] = u;
    n[7] = v;
    n[8] = w;
    for(int ax=0; ax<9; ax++) o[ax] -= g5xoffset[ax];
    unrotate(o[0], o[1], rotation_cos, rotation_sin);
    for(int ax=0; ax<9; ax++) o[ax] -= g92offset[ax];

    double theta1 = atan2(o[Y]-cy, o[X]-cx);
    double theta2 = atan2(n[Y]-cy, n[X]-cx);

    if(rot < 0) {
        while(theta2 - theta1 > -CIRCLE_FUZZ) theta2 -= 2*M_PI;
    } else {
        while(theta2 - theta1 < CIRCLE_FUZZ) theta2 += 2*M_PI;
    }

    // if multi-turn, add the right number of full circles
    if(rot < -1) theta2 += 2*M_PI*(rot+1);
    if(rot > 1) theta2 += 2*M_PI*(rot-1);

    int steps = std::max(3, int(max_segments * fabs(theta1 - theta2) / M_PI));
    double rsteps = 1. / steps;
    PyObject *segs = PyList_New(steps);

    double dtheta = theta2 - theta1;
    double d[9] = {0, 0, 0, n[3]-o[3], n[4]-o[4], n[5]-o[5], n[6]-o[6], n[7]-o[7], n[8]-o[8]};
    d[Z] = n[Z] - o[Z];

    double tx = o[X] - cx, ty = o[Y] - cy, dc = cos(dtheta*rsteps), ds = sin(dtheta*rsteps);
    for(int i=0; i<steps-1; i++) {
        double f = (i+1) * rsteps;
        double p[9];
        rotate(tx, ty, dc, ds);
        p[X] = tx + cx;
        p[Y] = ty + cy;
        p[Z] = o[Z] + d[Z] * f;
        p[3] = o[3] + d[3] * f;
        p[4] = o[4] + d[4] * f;
        p[5] = o[5] + d[5] * f;
        p[6] = o[6] + d[6] * f;
        p[7] = o[7] + d[7] * f;
        p[8] = o[8] + d[8] * f;
        for(int ax=0; ax<9; ax++) p[ax] += g92offset[ax];
        rotate(p[0], p[1], rotation_cos, rotation_sin);
        for(int ax=0; ax<9; ax++) p[ax] += g5xoffset[ax];
        PyList_SET_ITEM(segs, i,
            Py_BuildValue("ddddddddd", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8]));
    }
    for(int ax=0; ax<9; ax++) n[ax] += g92offset[ax];
    rotate(n[0], n[1], rotation_cos, rotation_sin);
    for(int ax=0; ax<9; ax++) n[ax] += g5xoffset[ax];
    PyList_SET_ITEM(segs, steps-1,
        Py_BuildValue("ddddddddd", n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7], n[8]));
    return segs;
}

*/

