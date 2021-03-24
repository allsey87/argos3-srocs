/**
 * @file <argos3/plugins/robots/drone/simulator/pointmass3d_drone_model.cpp>
 *
 * @author Sinan Oguz - <soguz.ankara@gmail.com>
 * @author Michael Allwright - <allsey87@gmail.com>
 */

#include "pointmass3d_drone_model.h"
#include <argos3/core/utility/logging/argos_log.h>
#include <argos3/core/utility/math/cylinder.h>
#include <argos3/core/utility/math/range.h>
#include <argos3/core/simulator/simulator.h>
#include <argos3/core/simulator/space/space.h>
#include <argos3/plugins/simulator/physics_engines/pointmass3d/pointmass3d_engine.h>
#include <argos3/plugins/robots/drone/simulator/drone_flight_system_entity.h>
#include <argos3/plugins/robots/drone/simulator/drone_entity.h>

namespace argos
{

   /****************************************/
   /****************************************/

   CPointMass3DDroneModel::CPointMass3DDroneModel(CPointMass3DEngine &c_engine,
                                                  CDroneEntity &c_drone) :
      CPointMass3DModel(c_engine, c_drone.GetEmbodiedEntity()),
      m_cFlightSystemEntity(c_drone.GetFlightSystemEntity()) {
      /* register the origin anchor update method */
      RegisterAnchorMethod(GetEmbodiedEntity().GetOriginAnchor(),
                           &CPointMass3DDroneModel::UpdateOriginAnchor);
      /* reset the model */
      Reset();
   }

   /****************************************/
   /****************************************/

   void CPointMass3DDroneModel::Reset() {
      /* reset the drone's position */
      m_cPosition = GetEmbodiedEntity().GetOriginAnchor().Position;
      /* reset the home position (may have been changed by MoveTo or by added noise) */
      m_cHomePosition = m_cPosition;
      /* reset the drone's orientation */
      CRadians cYaw, cPitch, cRoll;
      GetEmbodiedEntity().GetOriginAnchor().Orientation.ToEulerAngles(cYaw, cPitch, cRoll);
      m_cOrientation.SetX(cRoll.GetValue());
      m_cOrientation.SetY(cPitch.GetValue());
      m_cOrientation.SetZ(cYaw.GetValue());
      /* reset the home yaw angle (may have been changed by MoveTo or by added noise) */
      m_fHomeYawAngle = cYaw.GetValue();
      /* reset the drone's velocity */
      m_cVelocity.Set(0.0, 0.0, 0.0);
      m_cVelocityPrev.Set(0.0, 0.0, 0.0);
      /* reset the drone's angular velocity */
      m_cAngularVelocity.Set(0.0, 0.0, 0.0);
      m_cPrevAngularVelocity.Set(0.0, 0.0, 0.0);
      /* reset the drone's previous acceleration */
      m_cPrevAcceleration.Set(0.0, 0.0, 0.0);
      /* reset the drone's previous angular acceleration */
      m_cPrevAngularAcceleration.Set(0.0, 0.0, 0.0);
      /* reset the variables for PID controller  */
      m_cPrevOrientationTarget.Set(0.0, 0.0, 0.0);
      m_cAngularVelCumulativeError.Set(0.0, 0.0, 0.0);
      m_fAltitudeCumulativeError = 0.0;
      m_fPrevTargetAltitude = 0.0;
      /* reset the gyro bias */   
      m_fGyroBias = GYRO_BIAS;
      m_fAccelBias = ACCEL_BIAS;   
      m_fARW = RW_GYRO;
      m_fVRW = RW_ACCEL;
      m_pcRNG = CRandom::CreateRNG("argos");
      /* update the axis-aligned bounding box, anchors, and entities */
      UpdateEntityStatus();
   }

   /****************************************/
   /****************************************/

   void CPointMass3DDroneModel::UpdateEntityStatus() {
      /* push sensor data to the control interface */
      /* TODO: model the actual sensor behavior and take modeled sensor data */
      m_cFlightSystemEntity.SetPositionReading(m_cPosition - m_cHomePosition);
      m_cFlightSystemEntity.SetOrientationReading(m_cOrientation - (CVector3::Z * m_fHomeYawAngle));
      m_cFlightSystemEntity.SetVelocityReading(m_cVelocity);
      m_cFlightSystemEntity.SetAngularVelocityReading(m_cAngularVelocity);
      /* update the axis-aligned bounding box, anchors, and entities */
      CPointMass3DModel::UpdateEntityStatus();
   }

   /****************************************/
   /****************************************/

   void CPointMass3DDroneModel::UpdateFromEntityStatus() {
      /* pull actuator data from the control interface */
      /* CDroneFlightSystemEntity: local coordinate system of the drone */
      /* CPointMass3DDroneModel: global coordinate system */
      CVector3 cTargetPosition(m_cFlightSystemEntity.GetTargetPosition());
      m_cInputPosition =
         m_cHomePosition + cTargetPosition.RotateZ(CRadians(m_fHomeYawAngle));
      m_fInputYawAngle =
         m_fHomeYawAngle + m_cFlightSystemEntity.GetTargetYawAngle().GetValue();
   }

   /****************************************/
   /****************************************/

   void CPointMass3DDroneModel::UpdatePhysics() {
      /* update the position (XY) and altitude (Z) controller */
      CVector3 cPositionError(m_cInputPosition - m_cPosition);
      /* accumulate the altitude error */
      m_fAltitudeCumulativeError += cPositionError.GetZ() * GetPM3DEngine().GetPhysicsClockTick();
      /* calculate the azimuth contribution to the navigation of drone on XY plane */
      Real fAzimuth = std::atan2(std::abs(cPositionError.GetY()),
                                 std::abs(cPositionError.GetX()));
      /* calculate velocity limits */
      CRange<Real> cVelocityLimitX(-XY_VEL_MAX * std::cos(fAzimuth), XY_VEL_MAX * std::cos(fAzimuth));
      CRange<Real> cVelocityLimitY(-XY_VEL_MAX * std::sin(fAzimuth), XY_VEL_MAX * std::sin(fAzimuth));
      CRange<Real> cVelocityLimitZ(-Z_VEL_MAX, Z_VEL_MAX);
      /* calculate desired XYZ velocities */
      Real fTargetTransVelX =
         cPositionError.GetX() * XY_POS_KP;
      Real fTargetTransVelY =
         cPositionError.GetY() * XY_POS_KP;
      Real fTargetTransVelZ =
         (m_cInputPosition.GetZ() - m_fPrevTargetAltitude) / GetPM3DEngine().GetPhysicsClockTick();
      /* saturate velocities */
      cVelocityLimitX.TruncValue(fTargetTransVelX);
      cVelocityLimitY.TruncValue(fTargetTransVelY);
      cVelocityLimitZ.TruncValue(fTargetTransVelZ);
      /* store the previous desired altitude for the altitude PID calculation */
      m_fPrevTargetAltitude = m_cInputPosition.GetZ();
      /* store XYZ velocity error */
      CVector3 cTransVelocityError(fTargetTransVelX, fTargetTransVelY, fTargetTransVelZ);
      cTransVelocityError -= m_cVelocity;
      /* calculate XY desired accelerations */
      CVector3 cTargetTransAcc = cTransVelocityError * XY_VEL_KP;
      /* yaw angle correction to get downward Z axis in the right handed coordinate system */
      Real fYawAngleCorrected(m_cOrientation.GetZ() + M_PI);
      /* outputs of the position controller */
      Real fDesiredRollAngle = std::cos(m_cOrientation.GetY()) * std::cos(m_cOrientation.GetX()) *
         (std::sin(fYawAngleCorrected) * cTargetTransAcc.GetX() - std::cos(fYawAngleCorrected) * cTargetTransAcc.GetY()) / m_cPM3DEngine.GetGravity();
      Real fDesiredPitchAngle = std::cos(m_cOrientation.GetY()) * std::cos(m_cOrientation.GetX()) *
         (std::cos(fYawAngleCorrected) * cTargetTransAcc.GetX() + std::sin(fYawAngleCorrected) * cTargetTransAcc.GetY()) / m_cPM3DEngine.GetGravity();
      Real fDesiredYawAngle = m_fInputYawAngle;
      /* saturate the outputs of the position controller */
      ROLL_PITCH_LIMIT.TruncValue(fDesiredRollAngle);
      ROLL_PITCH_LIMIT.TruncValue(fDesiredPitchAngle);
      /* store the desired orientation values in a  vector */
      CVector3 cOrientationTarget(fDesiredRollAngle, fDesiredPitchAngle, fDesiredYawAngle);
      /* output of the altitude controller */
      Real fAltitudeControlSignal = MASS * GetPM3DEngine().GetGravity() + 
         CalculatePIDResponse(cPositionError.GetZ(),
                              m_fAltitudeCumulativeError,
                              cTransVelocityError.GetZ(),
                              ALTITUDE_KP,
                              ALTITUDE_KI,
                              ALTITUDE_KD) / (std::cos(m_cOrientation.GetX()) * std::cos(m_cOrientation.GetY()));
      /*** attitude (roll, pitch, yaw) control ***/
      /* roll, pitch, yaw errors */
      CVector3 cOrientationError(cOrientationTarget - m_cOrientation);
      /* desired  roll, pitch, yaw rates */
      CVector3 cAngularVelocityTarget =
         (cOrientationTarget - m_cPrevOrientationTarget) / GetPM3DEngine().GetPhysicsClockTick();
      /* previous desired roll, pitch, yaw values for the controllers */
      m_cPrevOrientationTarget = cOrientationTarget;
      /* rotational rate errors */
      CVector3 cAngularVelocityError(cAngularVelocityTarget - m_cAngularVelocity);
      /* accumulated roll, pitch, yaw errors for the controllers */
      m_cAngularVelCumulativeError +=
         cAngularVelocityError * GetPM3DEngine().GetPhysicsClockTick();
      /* roll controller output signal */
      Real fAttitudeControlSignalX = INERTIA.GetX() *
         CalculatePIDResponse(cOrientationError.GetX(),
                              m_cAngularVelCumulativeError.GetX(),
                              cAngularVelocityError.GetX(),
                              ROLL_PITCH_KP,
                              ROLL_PITCH_KI,
                              ROLL_PITCH_KD);
      /* pitch controller output signal */
      Real fAttitudeControlSignalY = INERTIA.GetY() * 
         CalculatePIDResponse(cOrientationError.GetY(),
                              m_cAngularVelCumulativeError.GetY(),
                              cAngularVelocityError.GetY(),
                              ROLL_PITCH_KP,
                              ROLL_PITCH_KI,
                              ROLL_PITCH_KD);
      /* yaw controller output signal */
      Real fAttitudeControlSignalZ = INERTIA.GetZ() * 
         CalculatePIDResponse(cOrientationError.GetZ(),
                              m_cAngularVelCumulativeError.GetZ(),
                              cAngularVelocityError.GetZ(),
                              YAW_KP,
                              YAW_KI,
                              YAW_KD);
      /* saturate controller outputs */
      TORQUE_LIMIT.TruncValue(fAttitudeControlSignalX);
      TORQUE_LIMIT.TruncValue(fAttitudeControlSignalY);
      TORQUE_LIMIT.TruncValue(fAttitudeControlSignalZ);
      /* calculate the rotor speeds from the control signals */
      std::array<Real, 4> arrSquaredRotorSpeeds = {
         fAltitudeControlSignal / (4 * B) - 
            fAttitudeControlSignalX * (ROOT_TWO / (4 * B * ARM_LENGTH)) -
            fAttitudeControlSignalY * (ROOT_TWO / (4 * B * ARM_LENGTH)) -
            fAttitudeControlSignalZ / (4 * D),
         fAltitudeControlSignal / (4 * B) -
            fAttitudeControlSignalX * (ROOT_TWO / (4 * B * ARM_LENGTH)) +
            fAttitudeControlSignalY * (ROOT_TWO / (4 * B * ARM_LENGTH)) +
            fAttitudeControlSignalZ / (4 * D),
         fAltitudeControlSignal / (4 * B) +
            fAttitudeControlSignalX * (ROOT_TWO / (4 * B * ARM_LENGTH)) +
            fAttitudeControlSignalY * (ROOT_TWO / (4 * B * ARM_LENGTH)) -
            fAttitudeControlSignalZ / (4 * D),
         fAltitudeControlSignal / (4 * B) +
            fAttitudeControlSignalX * (ROOT_TWO / (4 * B * ARM_LENGTH)) -
            fAttitudeControlSignalY * (ROOT_TWO / (4 * B * ARM_LENGTH)) +
            fAttitudeControlSignalZ / (4 * D),
      };
      /* calculate the effect of the rotor speed on the drone dynamics*/
      Real fOmegaR = -std::sqrt(std::abs(arrSquaredRotorSpeeds[0])) + 
                      std::sqrt(std::abs(arrSquaredRotorSpeeds[1])) +
                     -std::sqrt(std::abs(arrSquaredRotorSpeeds[2])) +
                      std::sqrt(std::abs(arrSquaredRotorSpeeds[3]));
      /* calculate the drone thrusts and torques */
      Real fThrust = B *
         ( arrSquaredRotorSpeeds[0] + arrSquaredRotorSpeeds[1] + arrSquaredRotorSpeeds[2] + arrSquaredRotorSpeeds[3]);
      Real fTorqueX = (ARM_LENGTH / ROOT_TWO) * B *
         (-arrSquaredRotorSpeeds[0] - arrSquaredRotorSpeeds[1] + arrSquaredRotorSpeeds[2] + arrSquaredRotorSpeeds[3]);
      Real fTorqueY = (ARM_LENGTH / ROOT_TWO) * B *
         (-arrSquaredRotorSpeeds[0] + arrSquaredRotorSpeeds[1] + arrSquaredRotorSpeeds[2] - arrSquaredRotorSpeeds[3]);
      Real fTorqueZ = D *
         (-arrSquaredRotorSpeeds[0] + arrSquaredRotorSpeeds[1] - arrSquaredRotorSpeeds[2] + arrSquaredRotorSpeeds[3]);
      /* saturate the thrust */
      THRUST_LIMIT.TruncValue(fThrust);
      /* calculate the system response: acceleration */
      CVector3 cAcceleration {
         (std::cos(fYawAngleCorrected) * std::sin(m_cOrientation.GetY()) * std::cos(m_cOrientation.GetX()) +
          std::sin(fYawAngleCorrected) * std::sin(m_cOrientation.GetX())) * fThrust / MASS,
         (std::sin(fYawAngleCorrected) * std::sin(m_cOrientation.GetY()) * std::cos(m_cOrientation.GetX()) -
          std::cos(fYawAngleCorrected) * std::sin(m_cOrientation.GetX())) * fThrust / MASS,
         -m_cPM3DEngine.GetGravity() + (std::cos(m_cOrientation.GetY()) * std::cos(m_cOrientation.GetX()) * fThrust / MASS)
      };
      /* calculate the system response: angular acceleration */
      CVector3 cAngularAcceleration {
         (fTorqueX / INERTIA.GetX()) -
         (JR * m_cAngularVelocity.GetY() * fOmegaR / INERTIA.GetX()) +
         (m_cAngularVelocity.GetZ() * m_cAngularVelocity.GetY() * (INERTIA.GetY() - INERTIA.GetZ()) / INERTIA.GetX()),
         (fTorqueY / INERTIA.GetY()) +
         (JR * m_cAngularVelocity.GetX() * fOmegaR / INERTIA.GetY()) +
         (m_cAngularVelocity.GetZ() * m_cAngularVelocity.GetX() * (INERTIA.GetZ() - INERTIA.GetX()) / INERTIA.GetY()),
         (fTorqueZ / INERTIA.GetZ()) +
         (m_cAngularVelocity.GetY() * m_cAngularVelocity.GetX() * (INERTIA.GetX() - INERTIA.GetY()) / INERTIA.GetZ())
      };
      /* gyroscope sensor readings */
      cAngularAcceleration.Set(
         
         cAngularAcceleration.GetX() + m_pcRNG->Gaussian(STD_GYRO_X,MEAN_SENS) + m_fGyroBias,
         cAngularAcceleration.GetY() + m_pcRNG->Gaussian(STD_GYRO_Y,MEAN_SENS) + m_fGyroBias,
         cAngularAcceleration.GetZ() + m_pcRNG->Gaussian(STD_GYRO_Z,MEAN_SENS) + m_fGyroBias
      );
      m_fGyroBias = m_fGyroBias + m_fARW *  m_pcRNG->Gaussian(STD_BIAS, MEAN_SENS);
      /* update the gyro bias angular random walk */
      m_fARW =  m_fARW * std::sqrt(GetPM3DEngine().GetPhysicsClockTick());
      /* update the angular velocity using trapezoid integration */
      cAngularAcceleration = 0.5 * (m_cPrevAngularAcceleration + cAngularAcceleration);
      m_cAngularVelocity += cAngularAcceleration * GetPM3DEngine().GetPhysicsClockTick();
      m_cPrevAngularAcceleration = cAngularAcceleration;
      /* update the orientation using trapezoid integration */
      m_cAngularVelocity = 0.5 * (m_cPrevAngularVelocity + m_cAngularVelocity);
      m_cOrientation += m_cAngularVelocity * GetPM3DEngine().GetPhysicsClockTick();
      m_cPrevAngularVelocity = m_cAngularVelocity;
      /* accelerometer sensor readings */
      cAcceleration.Set(
         cAcceleration.GetX() + m_pcRNG->Gaussian(STD_ACCEL_X,MEAN_SENS) + m_fAccelBias,
         cAcceleration.GetY() + m_pcRNG->Gaussian(STD_ACCEL_Y,MEAN_SENS) + m_fAccelBias,
         cAcceleration.GetZ() + m_pcRNG->Gaussian(STD_ACCEL_Z,MEAN_SENS) + m_fAccelBias
      );
      m_fAccelBias = m_fAccelBias + m_fVRW *  m_pcRNG->Gaussian(STD_BIAS, MEAN_SENS);
      /* update the accel bias velocity random walk */
      m_fVRW =  m_fVRW * std::sqrt(GetPM3DEngine().GetPhysicsClockTick());
      /* update the velocity using trapezoid integration */
      cAcceleration = 0.5 * (m_cPrevAcceleration + cAcceleration);
      m_cVelocity += cAcceleration * GetPM3DEngine().GetPhysicsClockTick();
      m_cPrevAcceleration = cAcceleration;
      /* update the position using trapezoid integration */
      m_cVelocity = 0.5 * (m_cVelocityPrev + m_cVelocity);
      m_cPosition += m_cVelocity * GetPM3DEngine().GetPhysicsClockTick();
      m_cVelocityPrev = m_cVelocity;
      /* TODO: position and attitude estimation and accel/gyro sensor fusion */

   }

   /****************************************/
   /****************************************/

   Real CPointMass3DDroneModel::CalculatePIDResponse(Real f_cur_error, Real f_sum_error, Real f_vel_error,
                                                     Real f_k_p, Real f_k_i, Real f_k_d) {
      return (f_k_p * f_cur_error + f_k_i * f_sum_error + f_k_d * f_vel_error);
   }

   /****************************************/
   /****************************************/

   void CPointMass3DDroneModel::MoveTo(const CVector3 &c_position,
                                       const CQuaternion &c_orientation) {
      /* convert to Euler angles */
      CRadians cYaw, cPitch, cRoll;
      c_orientation.ToEulerAngles(cYaw, cPitch, cRoll);
      /* calculate the requested change in position and yaw */
      Real fDeltaYaw = cYaw.GetValue() - m_cOrientation.GetZ();
      /* update the home yaw and home position */
      m_fHomeYawAngle += fDeltaYaw;
      CVector3 cOffsetPosition(m_cHomePosition - m_cPosition);
      m_cHomePosition = c_position + cOffsetPosition.RotateZ(CRadians(fDeltaYaw));
      /* update the position and orientation */
      m_cPosition = c_position;
      m_cOrientation.Set(cRoll.GetValue(),
                         cPitch.GetValue(),
                         cYaw.GetValue());
      /* update the previous orientation target */
      m_cPrevOrientationTarget.SetZ(m_cPrevOrientationTarget.GetZ() + fDeltaYaw);
      /* update the space */
      UpdateEntityStatus();
   }

   /****************************************/
   /****************************************/

   void CPointMass3DDroneModel::CalculateBoundingBox() {
      /* TODO update this method to compensate for drone orientation */
      GetBoundingBox().MinCorner.Set(
         GetEmbodiedEntity().GetOriginAnchor().Position.GetX() - ARM_LENGTH,
         GetEmbodiedEntity().GetOriginAnchor().Position.GetY() - ARM_LENGTH,
         GetEmbodiedEntity().GetOriginAnchor().Position.GetZ());
      GetBoundingBox().MaxCorner.Set(
         GetEmbodiedEntity().GetOriginAnchor().Position.GetX() + ARM_LENGTH,
         GetEmbodiedEntity().GetOriginAnchor().Position.GetY() + ARM_LENGTH,
         GetEmbodiedEntity().GetOriginAnchor().Position.GetZ() + HEIGHT);
   }

   /****************************************/
   /****************************************/

   bool CPointMass3DDroneModel::CheckIntersectionWithRay(Real &f_t_on_ray,
                                                         const CRay3 &c_ray) const {
      const CQuaternion& cDroneOrientation = 
         GetEmbodiedEntity().GetOriginAnchor().Orientation;
      CVector3 cDroneAxis(CVector3::Z);
      cDroneAxis.Rotate(cDroneOrientation);
      /* create a cylinder shape for testing the intersection */
      CCylinder m_cShape(ARM_LENGTH,
                         HEIGHT,
                         m_cPosition,
                         cDroneAxis);
      return m_cShape.Intersects(f_t_on_ray, c_ray);
   }

   /****************************************/
   /****************************************/

   void CPointMass3DDroneModel::UpdateOriginAnchor(SAnchor &s_anchor) {
      s_anchor.Position = m_cPosition;
      s_anchor.Orientation.FromEulerAngles(CRadians(m_cOrientation.GetZ()),
                                           CRadians(m_cOrientation.GetY()),
                                           CRadians(m_cOrientation.GetX()));
   }

   /****************************************/
   /****************************************/

   /* drone parameters */
   const Real CPointMass3DDroneModel::HEIGHT = 0.25;
   const Real CPointMass3DDroneModel::ARM_LENGTH = 0.22;
   const Real CPointMass3DDroneModel::MASS = 0.812;
   const CVector3 CPointMass3DDroneModel::INERTIA = {0.01085, 0.01092, 0.02121};
   const Real CPointMass3DDroneModel::B = 1.1236e-5;
   const Real CPointMass3DDroneModel::D = 1.4088e-7;
   const Real CPointMass3DDroneModel::JR = 5.225e-5; 
   /* limits */
   const CRange<Real> CPointMass3DDroneModel::TORQUE_LIMIT = CRange<Real>(-0.5721, 0.5721);
   const CRange<Real> CPointMass3DDroneModel::THRUST_LIMIT = CRange<Real>(-15, 15);
   const CRange<Real> CPointMass3DDroneModel::ROLL_PITCH_LIMIT = CRange<Real>(-0.5, 0.5);
   const Real CPointMass3DDroneModel::XY_VEL_MAX = 1;
   const Real CPointMass3DDroneModel::Z_VEL_MAX = 0.05;
   /* PID coefficients */
   const Real CPointMass3DDroneModel::XY_POS_KP = 1;
   const Real CPointMass3DDroneModel::XY_VEL_KP = 3;
   const Real CPointMass3DDroneModel::YAW_KP = 13;
   const Real CPointMass3DDroneModel::YAW_KI = 0;
   const Real CPointMass3DDroneModel::YAW_KD = 8;
   const Real CPointMass3DDroneModel::ALTITUDE_KP = 5;
   const Real CPointMass3DDroneModel::ALTITUDE_KI = 0;
   const Real CPointMass3DDroneModel::ALTITUDE_KD = 6;   
   const Real CPointMass3DDroneModel::ROLL_PITCH_KP = 12;
   const Real CPointMass3DDroneModel::ROLL_PITCH_KI = 0;
   const Real CPointMass3DDroneModel::ROLL_PITCH_KD = 6;
   const Real CPointMass3DDroneModel::ROOT_TWO = std::sqrt(2.0);
   /* sensor noise coefficents*/
   const Real CPointMass3DDroneModel::MEAN_SENS = 0;
   const Real CPointMass3DDroneModel::STD_GYRO_X = 0.67;
   const Real CPointMass3DDroneModel::STD_GYRO_Y = 0.78;
   const Real CPointMass3DDroneModel::STD_GYRO_Z = 0.12;
   const Real CPointMass3DDroneModel::STD_ACCEL_X =  0.1;
   const Real CPointMass3DDroneModel::STD_ACCEL_Y = 0.1;
   const Real CPointMass3DDroneModel::STD_ACCEL_Z = 0.12;
   const Real CPointMass3DDroneModel::STD_BIAS = 1;
   const Real CPointMass3DDroneModel::GYRO_BIAS = 0.01;
   const Real CPointMass3DDroneModel::ACCEL_BIAS = 0.03;
   const Real CPointMass3DDroneModel::RW_GYRO = 0.001;
   const Real CPointMass3DDroneModel::RW_ACCEL = 0.003;

   /****************************************/
   /****************************************/





   REGISTER_STANDARD_POINTMASS3D_OPERATIONS_ON_ENTITY(CDroneEntity, CPointMass3DDroneModel);

   /****************************************/
   /****************************************/

}
