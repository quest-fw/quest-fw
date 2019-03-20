// ======================================================================
// \title  ActuatorAdapterImpl.cpp
// \author mereweth
// \brief  cpp file for ActuatorAdapter component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the Office
// of Technology Transfer at the California Institute of Technology.
//
// This software may be subject to U.S. export control laws and
// regulations.  By accepting this document, the user agrees to comply
// with all U.S. export laws and regulations.  User has the
// responsibility to obtain export licenses, or other export authority
// as may be required before exporting such information to foreign
// countries or providing access to foreign persons.
// ======================================================================


#include <Gnc/Ctrl/ActuatorAdapter/ActuatorAdapterComponentImpl.hpp>
#include "Fw/Types/BasicTypes.hpp"
#include "Fw/Types/SerialBuffer.hpp"

#include <math.h>
#include <string.h>

#ifndef M_PI
#ifdef BUILD_DSPAL
#define M_PI 3.14159265358979323846
#endif
#endif

#ifdef BUILD_DSPAL
#include <HAP_farf.h>
#define DEBUG_PRINT(x,...) FARF(ALWAYS,x,##__VA_ARGS__);
#else
#include <stdio.h>
#define DEBUG_PRINT(x,...) printf(x,##__VA_ARGS__); fflush(stdout)
#endif

#undef DEBUG_PRINT
#define DEBUG_PRINT(x,...)

namespace Gnc {

  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction
  // ----------------------------------------------------------------------

  ActuatorAdapterComponentImpl ::
#if FW_OBJECT_NAMES == 1
    ActuatorAdapterComponentImpl(
        const char *const compName
    ) :
      ActuatorAdapterComponentBase(compName),
#else
    ActuatorAdapterImpl(void),
#endif
    outputInfo(),
    armedState(DISARMED),
    opCode(0u),
    cmdSeq(0u),
    armCount(0u),
    numActuators(0u),
    paramsInited(false)
  {
      for (U32 i = 0; i < AA_MAX_ACTUATORS; i++) {
          this->outputInfo[i].type = OUTPUT_UNSET;
      }
  }

  void ActuatorAdapterComponentImpl ::
    init(
        const NATIVE_INT_TYPE instance
    )
  {
    ActuatorAdapterComponentBase::init(instance);
  }

  ActuatorAdapterComponentImpl ::
    ~ActuatorAdapterComponentImpl(void)
  {

  }

  bool ActuatorAdapterComponentImpl ::
    setupI2C(
             U32 actuator,
             I2CMetadata meta,
             bool useSimple
    )
  {
      if (actuator >= AA_MAX_ACTUATORS) {
          return false;
      }
      // TODO(mereweth) - factor out FeedbackMetadata and CmdOutputMapMetadata checking
      if (0 == meta.fbMeta.countsPerRev) {
          return false;
      }
      meta.fbMeta.kp = fabs(meta.fbMeta.kp);
      meta.fbMeta.maxErr = fabs(meta.fbMeta.maxErr);
      meta.cmdOutputMap.Vnom = fabs(meta.cmdOutputMap.Vnom);
      meta.cmdOutputMap.Vact = meta.cmdOutputMap.Vnom;

      if (useSimple) {
          this->outputInfo[actuator].type = OUTPUT_I2C_SIMPLE;
      }
      else {
          this->outputInfo[actuator].type = OUTPUT_I2C;
      }
      this->outputInfo[actuator].i2cMeta = meta;

      return true;
  }

  void ActuatorAdapterComponentImpl ::
    parameterUpdated(FwPrmIdType id)
  {
#ifndef BUILD_TIR5
    printf("prm %d updated\n", id);
#endif
  }

  void ActuatorAdapterComponentImpl ::
    parametersLoaded()
  {
      this->paramsInited = false;
      Fw::ParamValid valid[7];
      this->numActuators = paramGet_numActuators(valid[0]);
      if (Fw::PARAM_VALID != valid[0]) {  return;  }
      if (this->numActuators >= AA_MAX_ACTUATORS) {  return;  }

      // NOTE(mereweth) - start convenience defines
#define MAP_AND_FB_FROM_PARM_IDX(XXX) \
      { \
          memset(&cmdOutputMap, 0, sizeof(cmdOutputMap)); \
          memset(&fb, 0, sizeof(fb)); \
          cmdOutputMap.minIn = paramGet_p ## XXX ## _minCmd(valid[0]); \
          cmdOutputMap.maxIn = paramGet_p ## XXX ## _maxCmd(valid[1]); \
          fb.countsPerRev = paramGet_p ## XXX ## _counts(valid[2]); \
          fb.maxErr = paramGet_p ## XXX ## _ctrl_maxErr(valid[3]); \
          fb.ctrlType = (FeedbackCtrlType) paramGet_p ## XXX ## _ctrlType(valid[4]); \
          for (U32 j = 0; j < 5; j++) { \
              if (Fw::PARAM_VALID != valid[j]) {  return;  } \
          } \
          if ((fb.ctrlType < FEEDBACK_CTRL_VALID_MIN) || \
              (fb.ctrlType > FEEDBACK_CTRL_VALID_MAX)) { \
              return; \
          } \
          switch (fb.ctrlType) { \
              case FEEDBACK_CTRL_NONE: \
                  /* NOTE(mereweth) - nothing else to do*/ \
                  break; \
              case FEEDBACK_CTRL_PROP: \
                  fb.kp = paramGet_p ## XXX ## _ctrl_kp(valid[0]); \
                  if (Fw::PARAM_VALID != valid[0]) {  return;  } \
                  break; \
              default: \
                  DEBUG_PRINT("Unhandled feedback ctrl type %u\n", fb.ctrlType); \
                  return; \
          } \
          \
          cmdOutputMap.type = (CmdOutputMapType) paramGet_p ## XXX ## _mapType(valid[0]); \
          if ((Fw::PARAM_VALID != valid[0]) || \
              (cmdOutputMap.type < CMD_OUTPUT_MAP_VALID_MIN) || \
              (cmdOutputMap.type > CMD_OUTPUT_MAP_VALID_MAX)) { \
              return; \
          } \
          cmdOutputMap.Vnom = paramGet_p ## XXX ## _map_Vnom(valid[0]); \
          cmdOutputMap.Vact = cmdOutputMap.Vnom; \
          if (Fw::PARAM_VALID != valid[0]) {  return;  } \
          switch (cmdOutputMap.type) { \
              case CMD_OUTPUT_MAP_UNSET: \
                  DEBUG_PRINT("Command to output mapping type unset\n"); \
                  return; \
              case CMD_OUTPUT_MAP_LIN_MINMAX: \
                  /* NOTE(mereweth) - nothing else to do*/ \
                  break; \
              case CMD_OUTPUT_MAP_LIN_2BRK: \
                  cmdOutputMap.x1 = paramGet_p ## XXX ## _map_x1(valid[0]); \
                  cmdOutputMap.k2 = paramGet_p ## XXX ## _map_k2(valid[1]); \
                  for (U32 j = 0; j < 2; j++) { \
                      if (Fw::PARAM_VALID != valid[j]) {  return;  } \
                  } \
                  /* NOTE(mereweth) - fallthrough intended*/ \
              case CMD_OUTPUT_MAP_LIN_1BRK: \
                  cmdOutputMap.x0 = paramGet_p ## XXX ## _map_x0(valid[0]); \
                  cmdOutputMap.k1 = paramGet_p ## XXX ## _map_k1(valid[1]); \
                  for (U32 j = 0; j < 2; j++) { \
                      if (Fw::PARAM_VALID != valid[j]) {  return;  } \
                  } \
                  /* NOTE(mereweth) - fallthrough intended*/ \
              case CMD_OUTPUT_MAP_LIN_0BRK: \
                  cmdOutputMap.k0 = paramGet_p ## XXX ## _map_k0(valid[0]); \
                  cmdOutputMap.b = paramGet_p ## XXX ## _map_b(valid[1]); \
                  for (U32 j = 0; j < 2; j++) { \
                      if (Fw::PARAM_VALID != valid[j]) {  return;  } \
                  } \
                  break; \
              default: \
                  DEBUG_PRINT("Unhandled mapping type %u\n", cmdOutputMap.type); \
                  return; \
          } \
      }

#define I2C_FROM_PARM_IDX(XXX) \
      { \
          i2c.minOut = (U32) paramGet_p ## XXX ## _minOut(valid[0]); \
          i2c.maxOut = (U32) paramGet_p ## XXX ## _maxOut(valid[1]); \
          for (U32 j = 0; j < 2; j++) { \
              if (Fw::PARAM_VALID != valid[j]) {  return;  } \
          } \
      }

#define METADATA_FROM_ACT_IDX(XXX) \
      { \
          outType = (OutputType) paramGet_a ## XXX ## _type(valid[0]); \
          if ((Fw::PARAM_VALID != valid[0]) || \
              (outType < OUTPUT_VALID_MIN) || \
              (outType > OUTPUT_VALID_MAX)) { \
              return; \
          } \
          U8 parmSlot = paramGet_a ## XXX ## _parmSlot(valid[0]); \
          if (Fw::PARAM_VALID != valid[0]) {  return;  } \
          switch (outType) { \
              case OUTPUT_UNSET: \
                  DEBUG_PRINT("Actuator type unset\n"); \
                  return; \
              case OUTPUT_PWM: \
                  DEBUG_PRINT("Unhandled act adap type %u\n", outType); \
                  return; \
              case OUTPUT_I2C: \
              case OUTPUT_I2C_SIMPLE: \
                  i2c.addr = paramGet_a ## XXX ## _addr(valid[0]); \
                  i2c.reverse = paramGet_a ## XXX ## _reverse(valid[1]); \
                  for (U32 j = 0; j < 2; j++) { \
                      if (Fw::PARAM_VALID != valid[j]) {  return;  } \
                  } \
                  \
                  /* TODO(mereweth) - update when number of parm slots updates */ \
                  switch (parmSlot) { \
                      case 0: \
                          return; \
                      case 1: \
                          MAP_AND_FB_FROM_PARM_IDX(1); \
                          I2C_FROM_PARM_IDX(1); \
                          break; \
                      default: \
                          DEBUG_PRINT("Unhandled parm slot %u\n", parmSlot); \
                          return; \
                  } \
                  i2c.cmdOutputMap = cmdOutputMap; \
                  i2c.fbMeta = fb; \
                  \
                  if (!setupI2C(i, i2c, (OUTPUT_I2C_SIMPLE == outType))) { \
                      return; \
                  } \
                  break; \
              default: \
                  DEBUG_PRINT("Unknown act adap type %u\n", outType); \
                  FW_ASSERT(0, outType); \
                  return; \
          } \
      }
      // NOTE(mereweth) - end convenience defines

      OutputType outType = OUTPUT_UNSET;
      I2CMetadata i2c;
      FeedbackMetadata fb;
      CmdOutputMapMetadata cmdOutputMap;
      for (U32 i = 0; i < this->numActuators; i++) {
          switch (i) {
              case 0:
                  METADATA_FROM_ACT_IDX(1);
                  break;
              case 1:
                  METADATA_FROM_ACT_IDX(2);
                  break;
              case 2:
                  METADATA_FROM_ACT_IDX(3);
                  break;
              case 3:
                  METADATA_FROM_ACT_IDX(4);
                  break;
              case 4:
                  METADATA_FROM_ACT_IDX(5);
                  break;
              case 5:
                  METADATA_FROM_ACT_IDX(6);
                  break;
              default:
                  FW_ASSERT(0, i);
          }
      }

      this->paramsInited = true;
  }

  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  void ActuatorAdapterComponentImpl ::
    motor_handler(
        const NATIVE_INT_TYPE portNum,
        ROS::mav_msgs::Actuators &Actuators
    )
  {
      if ((ARMED != this->armedState) ||
          !paramsInited) {
          return;
      }

      U32 angVelCount = FW_MIN(Actuators.getangular_velocities_count(),
                               FW_MIN(AA_MAX_ACTUATORS,
                                      this->numActuators));
      NATIVE_INT_TYPE angVelSize = 0;
      const F64* angVels = Actuators.getangular_velocities(angVelSize);

      // send commands - lower latency
      for (U32 i = 0; i < FW_MIN(angVelCount, angVelSize); i++) {
          switch (this->outputInfo[i].type) {
              case OUTPUT_UNSET:
              {
                  //TODO(mereweth) - issue diagnostic that this is unset
              }
                  break;
              case OUTPUT_PWM:
              {
                  if (this->isConnected_pwmSetDuty_OutputPort(0)) {

                  }
                  else {
                      //TODO(mereweth) - issue error
                  }
              }
                  break;
              case OUTPUT_I2C:
              case OUTPUT_I2C_SIMPLE:
              {
                  if (this->isConnected_escConfig_OutputPort(0) &&
                      this->isConnected_escReadWrite_OutputPort(0)) {
                      Fw::Time cmdTime = this->getTime();

                      // TODO(mereweth) - put the I2C clock speed in config header? separate config ports?
                      this->escConfig_out(0, 400, this->outputInfo[i].i2cMeta.addr, 500);

                      I2CMetadata i2c = this->outputInfo[i].i2cMeta;
                      F64 inVal = angVels[i];
                      // NOTE(Mereweth) - DSPAL has no isnan
                      if (!(inVal < i2c.cmdOutputMap.maxIn) && 
                          !(inVal > i2c.cmdOutputMap.minIn)) {
                          // TODO(mereweth) - EVR about disarming due to NaN
                          inVal = i2c.cmdOutputMap.minIn;
                          this->armedState = DISARMED;
                      }
                      else if (inVal > i2c.cmdOutputMap.maxIn) {  inVal = i2c.cmdOutputMap.maxIn;  }
                      else if (inVal < i2c.cmdOutputMap.minIn) {  inVal = i2c.cmdOutputMap.minIn;  }
                      U32 out = 0u;
                      F64 des = 0.0;

                      // TODO(mereweth) - share mapping among output types
                      F64 Vnom_by_Vact = 1.0;
                      if (i2c.cmdOutputMap.Vact > 1e-3) {
                          Vnom_by_Vact = i2c.cmdOutputMap.Vnom / i2c.cmdOutputMap.Vact;
                          if ((Vnom_by_Vact < 0.1) ||
                              (Vnom_by_Vact > 10.0)) {
                              // TODO(mereweth) - EVR!!
                              Vnom_by_Vact = 1.0;
                          }
                      }
                      else {
                          // TODO(mereweth) - EVR!!
                      }
                      switch (i2c.cmdOutputMap.type) {
                          case CMD_OUTPUT_MAP_LIN_MINMAX:
                              out = (inVal - i2c.cmdOutputMap.minIn) /
                                (i2c.cmdOutputMap.maxIn - i2c.cmdOutputMap.minIn) * (i2c.maxOut - i2c.minOut)
                                + i2c.minOut;
                              break;
                          case CMD_OUTPUT_MAP_LIN_2BRK:
                              if (inVal < i2c.cmdOutputMap.x0) {
                                  des = i2c.cmdOutputMap.k0 * Vnom_by_Vact * inVal + i2c.cmdOutputMap.b;
                              }
                              else if (inVal < i2c.cmdOutputMap.x1) {
                                  des = i2c.cmdOutputMap.k0 * Vnom_by_Vact * i2c.cmdOutputMap.x0
                                    + i2c.cmdOutputMap.b
                                    + i2c.cmdOutputMap.k1 * Vnom_by_Vact * (inVal - i2c.cmdOutputMap.x0);
                              }
                              else {
                                  des = i2c.cmdOutputMap.k0 * Vnom_by_Vact * i2c.cmdOutputMap.x0
                                    + i2c.cmdOutputMap.b
                                    + i2c.cmdOutputMap.k1  * Vnom_by_Vact
                                                           * (i2c.cmdOutputMap.x1 - i2c.cmdOutputMap.x0)
                                    + i2c.cmdOutputMap.k2 * Vnom_by_Vact * (inVal - i2c.cmdOutputMap.x1);
                              }
                              if (des < 0.0) {  out = 0;  }
                              else {  out = (U32) des;  }
                              break;
                          case CMD_OUTPUT_MAP_LIN_1BRK:
                              if (inVal < i2c.cmdOutputMap.x0) {
                                  des = i2c.cmdOutputMap.k0 * Vnom_by_Vact * inVal + i2c.cmdOutputMap.b;
                              }
                              else {
                                  des = i2c.cmdOutputMap.k0 * Vnom_by_Vact * i2c.cmdOutputMap.x0
                                    + i2c.cmdOutputMap.b
                                    + i2c.cmdOutputMap.k1 * Vnom_by_Vact * (inVal - i2c.cmdOutputMap.x0);
                              }
                              if (des < 0.0) {  out = 0;  }
                              else {  out = (U32) des;  }
                              break;
                          case CMD_OUTPUT_MAP_LIN_0BRK:
                              des = i2c.cmdOutputMap.k0 * Vnom_by_Vact * inVal + i2c.cmdOutputMap.b;
                              if (des < 0.0) {  out = 0;  }
                              else {  out = (U32) des;  }
                              break;
                          case CMD_OUTPUT_MAP_UNSET:
                          default:
                              DEBUG_PRINT("Unhandled command output map slot %u\n", i2c.cmdOutputMap.type);
                              FW_ASSERT(0, i2c.cmdOutputMap.type);
                              break;
                      }

                      // TODO(mereweth) - share controller among output types
                      F64 delta = 0.0;
                      switch (i2c.fbMeta.ctrlType) {
                          case FEEDBACK_CTRL_PROP:
                              delta = i2c.fbMeta.kp * (inVal - this->outputInfo[i].feedback.angVel);
                              
                              // NOTE(Mereweth) - fallthrough so this code runs for all control types except NONE

                              // NOTE(Mereweth) - clamp the delta vs the open-loop mapping
                              if (delta > 0.0) {
                                delta = FW_MIN(delta, i2c.fbMeta.maxErr);
                              }
                              if (delta < 0.0) {
                                delta = -1.0 * FW_MIN(-1.0 * delta, i2c.fbMeta.maxErr);
                              }

                              if (delta > 0.0) {
                                  out += delta;
                              }
                              // have to be careful with signs
                              // delta < 0; |delta| < out; out + delta > 0
                              else if (fabs(delta) < (F64) out) {
                                  out = (U32) ((F64) out + delta);
                              }
                              // delta < 0; |delta| > out; out + delta < 0; so would overflow
                              else {
                                  out = 0u;
                              }
                              break;
                          case FEEDBACK_CTRL_NONE:
                          default:
                              break;
                      }          

                      DEBUG_PRINT("esc addr %u, in %f, out %u\n", i2c.addr, angVels[i], out);

                      if (out > i2c.maxOut) {  out = i2c.maxOut;  }
                      if (out < i2c.minOut) {  out = i2c.minOut;  }

                      Fw::Buffer readBufObj(0, 0, 0, 0); // no read
                      if (OUTPUT_I2C == this->outputInfo[i].type) {
                          // MSB is reverse bit
                          U8 writeBuf[3] = { 0x00,
                                             (U8) ((out / 8) | ((i2c.reverse ? 1 : 0) << 7)),
                                             (U8) (out % 8) };
                          Fw::Buffer writeBufObj(0, 0, (U64) writeBuf, FW_NUM_ARRAY_ELEMENTS(writeBuf));
                          this->escReadWrite_out(0, writeBufObj, readBufObj);
                      }
                      else { // simple protocol
                          U8 writeBuf[2] = { (U8) (out / 8), (U8) (out % 8) };
                          Fw::Buffer writeBufObj(0, 0, (U64) writeBuf, FW_NUM_ARRAY_ELEMENTS(writeBuf));
                          this->escReadWrite_out(0, writeBufObj, readBufObj);
                      }

                      this->outputInfo[i].feedback.cmdIn   = angVels[i];
                      this->outputInfo[i].feedback.cmd     = out;
                      this->outputInfo[i].feedback.cmdSec  = cmdTime.getSeconds();
                      this->outputInfo[i].feedback.cmdUsec = cmdTime.getUSeconds();
                  }
                  else {
                      //TODO(mereweth) - issue error
                  }
              }
                  break;
              default:
                  //TODO(mereweth) - DEBUG_PRINT
                  FW_ASSERT(0, this->outputInfo[i].type);
          }
      }

      // get feedback - higher latency OK
      for (U32 i = 0; i < FW_MIN(angVelCount, angVelSize); i++) {
          switch (this->outputInfo[i].type) {
              case OUTPUT_UNSET:
              case OUTPUT_PWM:
                  //NOTE(mereweth) - no way to get feedback
                  break;
              case OUTPUT_I2C:
              case OUTPUT_I2C_SIMPLE:
              {
                  if (this->isConnected_escConfig_OutputPort(0) &&
                      this->isConnected_escReadWrite_OutputPort(0)) {
                      Fw::Time fbTime = this->getTime();
                      F64 fbTimeLast = (F64) this->outputInfo[i].feedback.fbSec +
                        (F64) this->outputInfo[i].feedback.fbUsec * 0.001 * 0.001;
                      U32 countsLast = this->outputInfo[i].feedback.counts;

                      // TODO(mereweth) - put the I2C clock speed in config header? separate config ports?
                      this->escConfig_out(0, 400, this->outputInfo[i].i2cMeta.addr, 500);

                      U8 readBuf[9] = { 0 };
                      
                      if (OUTPUT_I2C == this->outputInfo[i].type) {
                          U8 writeBuf[1] = { 0x02 }; // start at rev_count_h
                          Fw::Buffer readBufObj(0, 0, (U64) readBuf, FW_NUM_ARRAY_ELEMENTS(readBuf));
                          Fw::Buffer writeBufObj(0, 0, (U64) writeBuf, FW_NUM_ARRAY_ELEMENTS(writeBuf));
                          this->escReadWrite_out(0, writeBufObj, readBufObj);
                      }
                      else { // simple protocol
                          Fw::Buffer readBufObj(0, 0, (U64) readBuf, 2);
                          Fw::Buffer writeBufObj(0, 0, 0, 0); // no write in simple protocol
                          this->escReadWrite_out(0, writeBufObj, readBufObj);
                      }
                      
                      if ((0xab == readBuf[8]) ||
                          (OUTPUT_I2C_SIMPLE == this->outputInfo[i].type)) {
                          this->outputInfo[i].feedback.counts      = (readBuf[0] << 8) + readBuf[1];
                          // TODO(mereweth) - establish units and validity
                          this->outputInfo[i].feedback.voltage     = (F32) ((readBuf[2] << 8) + readBuf[3]);
                          this->outputInfo[i].feedback.temperature = (F32) ((readBuf[4] << 8) + readBuf[5]);
                          this->outputInfo[i].feedback.current     = (F32) ((readBuf[6] << 8) + readBuf[7]);
                          this->outputInfo[i].feedback.fbSec       = fbTime.getSeconds();
                          this->outputInfo[i].feedback.fbUsec      = fbTime.getUSeconds();
                          
                          F64 fbTimeFloat = (F64) this->outputInfo[i].feedback.fbSec +
                            (F64) this->outputInfo[i].feedback.fbUsec * 0.001 * 0.001;

                          // guard against bad parameter setting and small time increment
                          if ((this->outputInfo[i].i2cMeta.fbMeta.countsPerRev > 0) && 
                              (fbTimeFloat - fbTimeLast > 1e-4)) {

                              if (OUTPUT_I2C == this->outputInfo[i].type) {

                                  this->outputInfo[i].feedback.angVel = 2.0 * M_PI
                                    * this->outputInfo[i].feedback.counts
                                    / (fbTimeFloat - fbTimeLast) / this->outputInfo[i].i2cMeta.fbMeta.countsPerRev;
                              }
                              else {
                                  U32 countsDiff = 0u;
                                  //To account for the CSC rolling over at 0xFFFF = 2^16-1
                                  if (countsLast > this->outputInfo[i].feedback.counts) {
                                      countsDiff = 0xFFFF - countsLast + this->outputInfo[i].feedback.counts;
                                  }
                                  else {
                                      countsDiff = this->outputInfo[i].feedback.counts - countsLast;
                                  }

                                  this->outputInfo[i].feedback.angVel = 2.0 * M_PI * countsDiff
                                    / (fbTimeFloat - fbTimeLast) / this->outputInfo[i].i2cMeta.fbMeta.countsPerRev;
                              }

                              // TODO(mereweth) - run rate estimator here

                              switch (i) {
                                  case 0:
                                      this->tlmWrite_ACTADAP_Rot0(this->outputInfo[i].feedback.angVel);
                                      this->tlmWrite_ACTADAP_Cmd0(this->outputInfo[i].feedback.cmd);
                                      this->tlmWrite_ACTADAP_CmdVel0(angVels[i]);
                                      break;
                                  case 1:
                                      this->tlmWrite_ACTADAP_Rot1(this->outputInfo[i].feedback.angVel);
                                      this->tlmWrite_ACTADAP_Cmd1(this->outputInfo[i].feedback.cmd);
                                      this->tlmWrite_ACTADAP_CmdVel1(angVels[i]);
                                      break;
                                  case 2:
                                      this->tlmWrite_ACTADAP_Rot2(this->outputInfo[i].feedback.angVel);
                                      this->tlmWrite_ACTADAP_Cmd2(this->outputInfo[i].feedback.cmd);
                                      this->tlmWrite_ACTADAP_CmdVel2(angVels[i]);
                                      break;
                                  case 3:
                                      this->tlmWrite_ACTADAP_Rot3(this->outputInfo[i].feedback.angVel);
                                      this->tlmWrite_ACTADAP_Cmd3(this->outputInfo[i].feedback.cmd);
                                      this->tlmWrite_ACTADAP_CmdVel3(angVels[i]);
                                      break;
                                  case 4:
                                      this->tlmWrite_ACTADAP_Rot4(this->outputInfo[i].feedback.angVel);
                                      this->tlmWrite_ACTADAP_Cmd4(this->outputInfo[i].feedback.cmd);
                                      this->tlmWrite_ACTADAP_CmdVel4(angVels[i]);
                                      break;
                                  case 5:
                                      this->tlmWrite_ACTADAP_Rot5(this->outputInfo[i].feedback.angVel);
                                      this->tlmWrite_ACTADAP_Cmd5(this->outputInfo[i].feedback.cmd);
                                      this->tlmWrite_ACTADAP_CmdVel5(angVels[i]);
                                      break;
                                  default:
                                      break;
                              }
                          }
                      }
                      
                      Fw::SerializeStatus status;
                      // sec, usec, motor id, command, response
                      U8 buff[sizeof(U8) + 2 * sizeof(U32) + sizeof(U32) + sizeof(F64)
                              + 2 * sizeof(U32) + sizeof(F64)
                              + sizeof(readBuf) + sizeof(U32)];
                      Fw::SerialBuffer buffObj(buff, FW_NUM_ARRAY_ELEMENTS(buff));
                      status = buffObj.serialize((U8) i);
                      FW_ASSERT(Fw::FW_SERIALIZE_OK == status,static_cast<AssertArg>(status));

                      status = buffObj.serialize(this->outputInfo[i].feedback.cmdSec);
                      FW_ASSERT(Fw::FW_SERIALIZE_OK == status,static_cast<AssertArg>(status));
                      status = buffObj.serialize(this->outputInfo[i].feedback.cmdUsec);
                      FW_ASSERT(Fw::FW_SERIALIZE_OK == status,static_cast<AssertArg>(status));
                      status = buffObj.serialize(this->outputInfo[i].feedback.cmd);
                      FW_ASSERT(Fw::FW_SERIALIZE_OK == status,static_cast<AssertArg>(status));
                      status = buffObj.serialize(this->outputInfo[i].feedback.cmdIn);
                      FW_ASSERT(Fw::FW_SERIALIZE_OK == status,static_cast<AssertArg>(status));

                      status = buffObj.serialize(this->outputInfo[i].feedback.fbSec);
                      FW_ASSERT(Fw::FW_SERIALIZE_OK == status,static_cast<AssertArg>(status));
                      status = buffObj.serialize(this->outputInfo[i].feedback.fbUsec);
                      FW_ASSERT(Fw::FW_SERIALIZE_OK == status,static_cast<AssertArg>(status));
                      status = buffObj.serialize(this->outputInfo[i].feedback.angVel);
                      FW_ASSERT(Fw::FW_SERIALIZE_OK == status,static_cast<AssertArg>(status));
                      NATIVE_INT_TYPE size = FW_NUM_ARRAY_ELEMENTS(readBuf);
                      status = buffObj.serialize(readBuf, size, false); // serialize length
                      FW_ASSERT(FW_NUM_ARRAY_ELEMENTS(readBuf) == size, size);
                      FW_ASSERT(Fw::FW_SERIALIZE_OK == status,static_cast<AssertArg>(status));

                      if (this->isConnected_serialDat_OutputPort(0)) {
                          this->serialDat_out(0, buffObj);
                      }
                  }
                  else {
                      //TODO(mereweth) - issue error
                  }
              }
                  break;
              default:
                  //TODO(mereweth) - DEBUG_PRINT
                  FW_ASSERT(0, this->outputInfo[i].type);
          }
      }
  }

  void ActuatorAdapterComponentImpl ::
    sched_handler(
        const NATIVE_INT_TYPE portNum,
        NATIVE_UINT_TYPE context
    )
  {
      if (ARMING == this->armedState) {
          if (!paramsInited) {
              this->armedState = DISARMED;
              this->cmdResponse_out(this->opCode, this->cmdSeq, Fw::COMMAND_EXECUTION_ERROR);
              return;
          }
          if (++this->armCount > AA_ARM_COUNT) {
              this->armedState = ARMED;
              this->cmdResponse_out(this->opCode, this->cmdSeq, Fw::COMMAND_OK);
              return;
          }
          for (U32 i = 0; i < FW_MIN(this->numActuators, AA_MAX_ACTUATORS); i++) {
              switch (this->outputInfo[i].type) {
                  case OUTPUT_UNSET:
                  {
                      //TODO(mereweth) - issue error
                  }
                      break;
                  case OUTPUT_PWM:
                  {
                      if (this->isConnected_pwmSetDuty_OutputPort(0)) {

                      }
                      else {
                          //TODO(mereweth) - issue error
                      }
                  }
                      break;
                  case OUTPUT_I2C:
                  case OUTPUT_I2C_SIMPLE:
                  {
                      if (this->isConnected_escConfig_OutputPort(0) &&
                          this->isConnected_escReadWrite_OutputPort(0)) {
                          // TODO(mereweth) - put the I2C clock speed in config header? separate config ports?
                          this->escConfig_out(0, 400, this->outputInfo[i].i2cMeta.addr, 500);

                          Fw::Buffer readBufObj(0, 0, 0, 0); // no read
                          if (OUTPUT_I2C == this->outputInfo[i].type) {
                              // MSB is reverse bit
                              U8 writeBuf[3] = { 0 };
                              Fw::Buffer writeBufObj(0, 0, (U64) writeBuf, FW_NUM_ARRAY_ELEMENTS(writeBuf));
                              this->escReadWrite_out(0, writeBufObj, readBufObj);
                          }
                          else { // simple protocol
                              U8 writeBuf[1] = { 0 };
                              Fw::Buffer writeBufObj(0, 0, (U64) writeBuf, FW_NUM_ARRAY_ELEMENTS(writeBuf));
                              this->escReadWrite_out(0, writeBufObj, readBufObj);
                          }
                      }
                      else {
                          //TODO(mereweth) - issue error
                      }
                  }
                      break;
                  default:
                      //TODO(mereweth) - DEBUG_PRINT
                      FW_ASSERT(0, this->outputInfo[i].type);
              }
          }
      }
  }

  // ----------------------------------------------------------------------
  // Command handler implementations
  // ----------------------------------------------------------------------

  void ActuatorAdapterComponentImpl ::
    ACTADAP_Arm_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        bool armState
    )
  {
      if (!this->paramsInited) {
          this->armedState = DISARMED;
          this->cmdResponse_out(this->opCode, this->cmdSeq, Fw::COMMAND_EXECUTION_ERROR);
          return;
      }
      // can only arm if disarmed
      if ((DISARMED != this->armedState) && armState) {
          this->log_WARNING_LO_ACTADAP_AlreadyArmed();
          this->cmdResponse_out(opCode, cmdSeq, Fw::COMMAND_EXECUTION_ERROR);
          return;
      }

      // if we get here, either we are disarmed now, or about to disarm
      this->armCount = 0u;

      // can disarm immediately - change this if hardware changes
      if (!armState) {
          this->armedState = DISARMED;
          this->cmdResponse_out(opCode, cmdSeq, Fw::COMMAND_OK);
          return;
      }

      // if we get here, we are disarmed, and want to arm
      this->armedState = ARMING;

      this->opCode = opCode;
      this->cmdSeq = cmdSeq;
  }

  void ActuatorAdapterComponentImpl ::
    ACTADAP_InitParams_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq
    )
  {
      this->parametersLoaded();
      if (this->paramsInited) {
          this->cmdResponse_out(opCode, cmdSeq, Fw::COMMAND_OK);
      }
      else {
          this->cmdResponse_out(opCode, cmdSeq, Fw::COMMAND_EXECUTION_ERROR);
      }
  }

  void ActuatorAdapterComponentImpl ::
    ACTADAP_SetVoltAct_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        U8 actIdx,
        F64 voltage
    )
  {
      if ((!this->paramsInited) ||
          (actIdx > this->numActuators) ||
          (voltage < 0.0)) {
          this->cmdResponse_out(opCode, cmdSeq, Fw::COMMAND_EXECUTION_ERROR);
          return;
      }

      switch (this->outputInfo[actIdx].type) {
          case OUTPUT_UNSET:
          {
              //TODO(mereweth) - EVR that this is unset
          }
              break;
          case OUTPUT_PWM:
          {
              this->outputInfo[actIdx].pwmMeta.cmdOutputMap.Vact = voltage;
          }
              break;
          case OUTPUT_I2C:
          case OUTPUT_I2C_SIMPLE:
          {
              this->outputInfo[actIdx].i2cMeta.cmdOutputMap.Vact = voltage;
          }
              break;
          default:
              //TODO(mereweth) - DEBUG_PRINT
              FW_ASSERT(0, this->outputInfo[actIdx].type);
      }

      this->cmdResponse_out(opCode, cmdSeq, Fw::COMMAND_OK);
  }
} // end namespace Gnc
