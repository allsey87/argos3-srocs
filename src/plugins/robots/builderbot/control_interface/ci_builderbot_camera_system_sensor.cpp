/**
 * @file <argos3/plugins/robots/builderbot/control_interface/ci_builderbot_camera_system_sensor.cpp>
 *
 * @author Michael Allwright <allsey87@gmail.com>
 */

#include "ci_builderbot_camera_system_sensor.h"

#ifdef ARGOS_WITH_LUA
#include <argos3/core/wrappers/lua/lua_utility.h>
#endif

namespace argos {

   /****************************************/
   /****************************************/

#ifdef ARGOS_WITH_LUA
   int LuaEnableBuilderBotCameraSystemSensor(lua_State* pt_lua_state) {
      /* Check parameters */
      if(lua_gettop(pt_lua_state) != 0) {
         return luaL_error(pt_lua_state, "robot.camera_system.enable() expects zero arguments");
      }
      /* Get the camera sensor */
      CCI_BuilderBotCameraSystemSensor* pcCameraSensor = 
         CLuaUtility::GetDeviceInstance<CCI_BuilderBotCameraSystemSensor>(pt_lua_state, "camera_system");
      /* Set the enable member */
      pcCameraSensor->Enable();
      return 0;
   }
#endif

   /****************************************/
   /****************************************/

#ifdef ARGOS_WITH_LUA
   int LuaDisableBuilderBotCameraSystemSensor(lua_State* pt_lua_state) {
      /* Check parameters */
      if(lua_gettop(pt_lua_state) != 0) {
         return luaL_error(pt_lua_state, "robot.camera_system.disable() expects zero arguments");
      }
      /* Get the camera sensor */
      CCI_BuilderBotCameraSystemSensor* pcCameraSensor = 
         CLuaUtility::GetDeviceInstance<CCI_BuilderBotCameraSystemSensor>(pt_lua_state, "camera_system");
      /* Set the enable member */
      pcCameraSensor->Disable();
      return 0;
   }
#endif

   /****************************************/
   /****************************************/
   
#ifdef ARGOS_WITH_LUA
   /*
    * The stack must have four numbers in this order:
    * 1. The x offset of the region to check (a number)
    * 2. The y offset of the region to check (a number)
    * 3. The width of the region to check (a number)
    * 4. The height of the region to check (a number)
    */
   int LuaBuilderBotCameraSystemSensorDetectLed(lua_State* pt_lua_state) {
      /* Check parameters */
      if(lua_gettop(pt_lua_state) != 4) {
         return luaL_error(pt_lua_state, "robot.camera_system.detect_led() expects 4 arguments");
      }
      luaL_checktype(pt_lua_state, 1, LUA_TNUMBER);
      luaL_checktype(pt_lua_state, 2, LUA_TNUMBER);
      luaL_checktype(pt_lua_state, 3, LUA_TNUMBER);
      luaL_checktype(pt_lua_state, 4, LUA_TNUMBER);
      /* Prepare the offset and patch size */
      CVector2 cOffset(lua_tonumber(pt_lua_state, 1),
                       lua_tonumber(pt_lua_state, 2));
      CVector2 cSize(lua_tonumber(pt_lua_state, 3),
                     lua_tonumber(pt_lua_state, 4));
      /* Get the camera sensor */
      CCI_BuilderBotCameraSystemSensor* pcCameraSensor = 
         CLuaUtility::GetDeviceInstance<CCI_BuilderBotCameraSystemSensor>(pt_lua_state, "camera_system");
      /* Get the color of the LED (CColor::BLACK if none) */
      const CColor& cColor = pcCameraSensor->DetectLed(cOffset, cSize);
      /* Create a table for the pixel data */
      lua_newtable (pt_lua_state);
      CLuaUtility::AddToTable(pt_lua_state, "_type", CLuaUtility::TYPE_COLOR);
      CLuaUtility::AddToTable(pt_lua_state, "red", cColor.GetRed());
      CLuaUtility::AddToTable(pt_lua_state, "green", cColor.GetGreen());
      CLuaUtility::AddToTable(pt_lua_state, "blue", cColor.GetBlue());
      /* return a single result, the table */
      return 1;
   }
#endif

   /****************************************/
   /****************************************/

#ifdef ARGOS_WITH_LUA
   void CCI_BuilderBotCameraSystemSensor::CreateLuaState(lua_State* pt_lua_state) {
      CLuaUtility::OpenRobotStateTable(pt_lua_state, "camera_system");
      CLuaUtility::AddToTable(pt_lua_state, "_instance", this);
      CLuaUtility::AddToTable(pt_lua_state,
                              "detect_led",
                              &LuaBuilderBotCameraSystemSensorDetectLed);
      CLuaUtility::AddToTable(pt_lua_state,
                              "enable",
                              &LuaEnableBuilderBotCameraSystemSensor);
      CLuaUtility::AddToTable(pt_lua_state,
                              "disable",
                              &LuaDisableBuilderBotCameraSystemSensor);
      CLuaUtility::AddToTable(pt_lua_state, "timestamp", 0.0f);
      CLuaUtility::StartTable(pt_lua_state, "tags");
      for(size_t i = 0; i < m_tTags.size(); ++i) {
         CLuaUtility::StartTable(pt_lua_state, i + 1);
         CLuaUtility::AddToTable(pt_lua_state, "payload", m_tTags[i].Payload);
         CLuaUtility::AddToTable(pt_lua_state, "center", m_tTags[i].Center);
         /* start corners */
         CLuaUtility::StartTable(pt_lua_state, "corners");
         for(size_t j = 0; j < m_tTags[i].Corners.size(); ++j) {           
            CLuaUtility::AddToTable(pt_lua_state, j + 1, m_tTags[i].Corners[j]);
         }
         CLuaUtility::EndTable(pt_lua_state);
         /* end corners */
         CLuaUtility::EndTable(pt_lua_state);
      }
      CLuaUtility::EndTable(pt_lua_state);
      CLuaUtility::CloseRobotStateTable(pt_lua_state);
   }
#endif

   /****************************************/
   /****************************************/

#ifdef ARGOS_WITH_LUA
   void CCI_BuilderBotCameraSystemSensor::ReadingsToLuaState(lua_State* pt_lua_state) {
      CLuaUtility::OpenRobotStateTable(pt_lua_state, "camera_system");
      CLuaUtility::AddToTable(pt_lua_state, "timestamp", m_fTimestamp);
      CLuaUtility::StartTable(pt_lua_state, "tags");
      /* get the tag count from last time */
      size_t unLastTagCount = lua_rawlen(pt_lua_state, -1);     
      for(size_t i = 0; i < m_tTags.size(); ++i) {
         CLuaUtility::StartTable(pt_lua_state, i + 1);
         CLuaUtility::AddToTable(pt_lua_state, "payload", m_tTags[i].Payload);
         CLuaUtility::AddToTable(pt_lua_state, "center", m_tTags[i].Center);
         CLuaUtility::StartTable(pt_lua_state, "corners");
         for(size_t j = 0; j < m_tTags[i].Corners.size(); ++j) {           
            CLuaUtility::AddToTable(pt_lua_state, j + 1, m_tTags[i].Corners[j]);
         }
         CLuaUtility::EndTable(pt_lua_state);
         CLuaUtility::EndTable(pt_lua_state);
      }
      if(m_tTags.size() < unLastTagCount) {
         /* Remove extra tags from the last update by setting them to nil */
         for(size_t i = m_tTags.size() + 1; i <= unLastTagCount; ++i) {
            lua_pushnumber(pt_lua_state,  i);
            lua_pushnil   (pt_lua_state    );
            lua_settable  (pt_lua_state, -3);
         }
      }
      CLuaUtility::EndTable(pt_lua_state);
      CLuaUtility::CloseRobotStateTable(pt_lua_state);
   }
#endif

   /****************************************/
   /****************************************/

}
