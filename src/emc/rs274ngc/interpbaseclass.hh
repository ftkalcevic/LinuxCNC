#ifndef INTERPBASE_HH
#define INTERPBASE_HH

#include "canon.hh"
#include <map>

#ifdef Log
#undef Log
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma message "warning disabled for this file"

class InterpBaseClass
{
public:
    InterpBaseClass( CANON_UNITS units = CANON_UNITS_MM );

    void parse_file( const char *filename );

    virtual void ArcFeed( int line_number,
                          double first_end, double second_end, double first_axis,
                          double second_axis, int rotation, double axis_end_point,
                          double a_position, double b_position, double c_position,
                          double u_position, double v_position, double w_position ) {}
    virtual void StraightFeed( int line_number,
                               double x, double y, double z,
                               double a, double b, double c,
                               double u, double v, double w ) {}
    virtual void StraightTraverse( int line_number,
                                   double x, double y, double z,
                                   double a, double b, double c,
                                   double u, double v, double w ) {}
    virtual void NurbsFeed(int line_number, std::vector<CONTROL_POINT> nurbs_control_points, unsigned int k) {}
    virtual void SetG5XOffset( int g5x_index,
                               double x, double y, double z,
                               double a, double b, double c,
                               double u, double v, double w ) {}
    virtual void SetG92Offset( double x, double y, double z,
                               double a, double b, double c,
                               double u, double v, double w ) {}
    virtual void SetXYRotation(double t) {}
    virtual void UseLengthUnits(CANON_UNITS u) {}
    virtual void SelectPlane(CANON_PLANE pl) {}
    virtual void SetTraverseRate(double rate) {}
    virtual void SetFeedMode(int mode) {}
    virtual void ChangeTool(int pocket) {}
    virtual void ChangeToolNumber(int pocket) {}
    virtual void SetFeedRate(double rate) {}
    virtual void Dwell(double time) {}
    virtual void Message(char *comment) {}
    virtual void Log(char *s) {}
    virtual void LogOpen(char *f) {}
    virtual void LogAppend(char *f) {}
    virtual void LogClose() {}
    virtual void Comment(const char *comment) {}
    virtual void SetToolTableEntry(int pocket, int toolno, EmcPose offset, double diameter,
                                      double frontangle, double backangle, int orientation) {}
    virtual void UseToolLengthOffset(const EmcPose &offset) {}
    virtual void SetFeedReference(double reference) {}
    virtual void SetCutterRadiusCompensation(double radius) {}
    virtual void StartCutterRadiusCompensation(int direction) {}
    virtual void StopCutterRadiusCompensation(int direction) {}
    virtual void StartSpeedFeedSynch() {}
    virtual void StartSpeedFeedSynch(double sync, bool vel) {}
    virtual void StopSpeedFeedSynch() {}
    virtual void StartSpindleCounterclockwise() {}
    virtual void StartSpindleClockwise() {}
    virtual void SetSpindleMode(double mode) {}
    virtual void StopSpindleTurning() {}
    virtual void SetSpindleSpeed(double rpm) {}
    virtual void OrientSpindle(double d, int i) {}
    virtual void WaitSpindleOrientComplete(double timeout) {}
    virtual void ProgramStop() {}
    virtual void ProgramEnd() {}
    virtual void Finish() {}
    virtual void PalletShuttle() {}
    virtual void SelectPocket(int pocket, int tool) {}
    virtual void OptionalProgramStop() {}
    virtual void StartChange() {}
    virtual int GetExternalTCFault() {return 0;} 
    virtual int GetExternalTCReason() {return 0;} 
    virtual bool GetBlockDelete() { return false; }
    virtual void CanonError(const char *fmt, va_list arg ) {}
    virtual void ClampAxis(CANON_AXIS axis) {}
    virtual bool GetOptionalProgramStop() { return false; }
    virtual void setOptionalProgramStop( bool state) {}
    virtual void SpindlRetractTraverse() {}
    virtual void SpindleRetract() {}
    virtual void StopCutterRadiusCompensation() {}
    virtual void UseNoSpindleForce() {}
    virtual void SetBlockDelete(bool enabled) {}
    virtual void DisableFeedOverride() {}
    virtual void DisableFeedHold() {}
    virtual void EnableFeedHold() {}
    virtual void DisableSpeedOverride() {}
    virtual void EnableFeedOverride() {}
    virtual void EnableSpeedOverride() {}
    virtual void MistOff() {}
    virtual void FloodOff() {}
    virtual void MistOn() {}
    virtual void FloodOn() {}
    virtual void ClearAuxOutputBit( int bit ) {}
    virtual void SetAuxOutputBit(int bit) {}
    virtual void SetAuxOutputValue(int index, double value) {}
    virtual void ClearMotionOutputBit(int bit) {}
    virtual void SetMotionOutputBit(int bit) {}
    virtual void SetMotionOutputValue(int index, double value) {}
    virtual void TurnProbeOn() {}
    virtual void TurnProbeOff() {}
    virtual int UnlockRotary(int line_no, int axis) {return 0;}
    virtual int LockRotary(int line_no, int axis) {return 0;}
    virtual void InterpAbort(int reason,const char *message) {}
    virtual void PluginCall(int len, const char *call) {}
    virtual void IOPluginCall(int len, const char *call) {}
    virtual void StraightProbe( int line_number, 
                                double x, double y, double z, 
                                double a, double b, double c,
                                double u, double v, double w, unsigned char probe_type ) {}
    virtual void RigidTap( int line_number,
                           double x, double y, double z) {}
    virtual void InitCanon() {}
    virtual void GetExternalParameterFileName(char *name, int max_size) {}
    virtual CANON_UNITS GetExternalLengthUnitType() { return CANON_UNITS_MM; }
    virtual CANON_TOOL_TABLE GetExternalToolTable(int pocket) { CANON_TOOL_TABLE t = {-1,-1,{{0,0,0},0,0,0,0,0,0},0,0,0,0}; return t; }
    virtual int GetExternalDigitalInput(int index, int def) { return def; }
    virtual int GetExternalAnalogInput(int index, double def) { return def; }
    virtual int Wait(int index, int input_type, int wait_type, double timeout) { return 0; }
    virtual void UserDefinedFunction(int num, double arg1, double arg2)  {}
    virtual void SetFeedReference( int ref ) {}
    virtual bool GetExternalQueueEmpty() { return true; }
    virtual CANON_DIRECTION GetExternalSpindle() { return CANON_STOPPED; }
    virtual int GetExternalToolSlot() { return 0; }
    virtual int GetExternalSelectedToolSlot() {return 0; }
    virtual double GetExternalFeedRate() { return 1.0; }
    virtual double GetExternalTraverseRate() { return 1.0; }
    virtual int GetExternalFlood() { return 0; }
    virtual int GetExternalMist() { return 0; }
    virtual CANON_PLANE GetExternalPlane() { return CANON_PLANE_XY; }
    virtual double GetExternalSpeed() { return 0; }
    virtual int GetExternalPocketsMax() { return CANON_POCKETS_MAX; }
    virtual void DisableAdaptiveFeed() {}
    virtual void EnableAdaptiveFeed() {}
    virtual int GetExternalFeedOverrideEnable() { return 1;}
    virtual int GetExternalSpindleOverrideEnable() { return 1;}
    virtual int GetExternalAdaptiveFeedEnable() {return 0;}
    virtual int GetExternalFeedHoldEnable() {return 1;}
    virtual int GetExternalAxisMask() { return (1<<(CANON_AXIS_X-1)) | (1<<(CANON_AXIS_Y-1)) | (1<<(CANON_AXIS_Z-1)); }
    virtual double GetExternalAngleUnits() { return 1.0; }
    virtual double GetExternalLengthUnits() { return 0.03937007874016;  /* 1/2.54 */ }
    virtual void NextLine( int sequence_number ) {}
    
    const EmcPose & CurrentPosRaw() const { return current_pos; }
    const EmcPose & ToolOffsetRaw() const { return tool_offset; }
    
    const EmcPose CurrentPos() const { EmcPose pos = current_pos; ScaleOutput(pos); return pos; }
    const EmcPose ToolOffset() const { EmcPose pos = tool_offset; ScaleOutput(pos); return pos; }
    const EmcPose &g92Offset() const { return g92_offset; }
    const EmcPose &g5xOffset() const { return g5x_offset; }
    int g5xIndex() const { return g5x_index; }
    CANON_PLANE Plane() const { return plane; }
    double XYRotation() const { return xy_rotation; }
    
private:
    EmcPose current_pos;  // Current position in raw units
    EmcPose g5x_offset;
    int g5x_index;
    EmcPose g92_offset;
    double xy_rotation;
    EmcPose tool_offset;
    int interp_error;
    int last_sequence_number;
    CANON_PLANE plane;
    CANON_UNITS outputUnits;
    bool bScaleOutput;
    double dOutputScale;

    void maybe_new_line(int sequence_number);
public:
    void _NurbsFeed(int line_number, std::vector<CONTROL_POINT> nurbs_control_points, unsigned int k);
    void _ArcFeed( int line_number,
                   double first_end, double second_end, double first_axis,
                   double second_axis, int rotation, double axis_end_point,
                   double a_position, double b_position, double c_position,
                   double u_position, double v_position, double w_position ) ;
    void _StraightFeed(int line_number,
                      double x, double y, double z,
                      double a, double b, double c,
                      double u, double v, double w);
    void _StraightTraverse( int line_number,
                            double x, double y, double z,
                            double a, double b, double c,
                            double u, double v, double w );
    void _SetG5XOffset( int g5x_index,
                        double x, double y, double z,
                        double a, double b, double c,
                        double u, double v, double w );
    void _SetG92Offset( double x, double y, double z,
                        double a, double b, double c,
                        double u, double v, double w );
    void _SetXYRotation(double t);
    void _UseLengthUnits(CANON_UNITS u);
    void _SelectPlane(CANON_PLANE pl);
    void _SetTraverseRate(double rate);
    void _SetFeedMode(int mode);
    void _ChangeTool(int pocket);
    void _ChangeToolNumber(int pocket);
    void _SetFeedRate(double rate) ;
    void _Dwell(double time) ;
    void _Message(char *comment);
    void _Comment(const char *comment);
    void _SetToolTableEntry(int pocket, int toolno, EmcPose offset, double diameter,
                            double frontangle, double backangle, int orientation) ;
    void _UseToolLengthOffset(EmcPose offset) ;
    void _StraightProbe( int line_number, 
                         double x, double y, double z, 
                         double a, double b, double c,
                         double u, double v, double w, unsigned char probe_type );
    void _RigidTap( int line_number,
                    double x, double y, double z );
    void _UserDefinedFunction(int num, double arg1, double arg2);
    
    
    void ScaleOutput( EmcPose &pos ) const;
    void ScaleOutput( double &a1 ) const;
    void ScaleOutput( double &a1, double &a2 ) const;
    void ScaleOutput( double &a1, double &a2, double &a3 ) const;
    void ScaleOutput( double &a1, double &a2, double &a3, double &a4, double &a5, double &a6 ) const;
    void ScaleOutput( double &a1, double &a2, double &a3, double &a4, double &a5, double &a6, double &a7, double &a8 ) const;
};
    
    

#pragma GCC diagnostic pop


#endif

