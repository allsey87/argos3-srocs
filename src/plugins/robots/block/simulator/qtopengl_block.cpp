/**
 * @file <argos3/plugins/robots/block/simulator/qtopengl_block.cpp>
 *
 * @author Michael Allwright - <allsey87@gmail.com>
 */

#include "qtopengl_block.h"
#include <argos3/core/utility/datatypes/color.h>
#include <argos3/core/utility/math/vector2.h>
#include <argos3/plugins/robots/block/simulator/block_entity.h>
#include <argos3/plugins/simulator/entities/directional_led_equipped_entity.h>
#include <argos3/plugins/simulator/entities/tag_equipped_entity.h>
#include <argos3/plugins/simulator/visualizations/qt-opengl/qtopengl_widget.h>

#define TAG_WHITE std::array<GLfloat, 3> {1.0f, 1.0f, 1.0f}
#define TAG_BLACK std::array<GLfloat, 3> {0.0f, 0.0f, 0.0f}

namespace argos {

   /****************************************/
   /****************************************/

   CQTOpenGLBlock::CQTOpenGLBlock() :
      /* create the model */
      m_cBlockModel("block.obj"),
      /* get pointers to the LED materials */
      m_arrLEDs {
         &m_cBlockModel.GetMaterial("led_0"),
         &m_cBlockModel.GetMaterial("led_1"),
         &m_cBlockModel.GetMaterial("led_2"),
         &m_cBlockModel.GetMaterial("led_3"),
         &m_cBlockModel.GetMaterial("led_4"),
         &m_cBlockModel.GetMaterial("led_5"),
         &m_cBlockModel.GetMaterial("led_6"),
         &m_cBlockModel.GetMaterial("led_7"),
         &m_cBlockModel.GetMaterial("led_8"),
         &m_cBlockModel.GetMaterial("led_9"),
         &m_cBlockModel.GetMaterial("led_10"),
         &m_cBlockModel.GetMaterial("led_11"),
         &m_cBlockModel.GetMaterial("led_12"),
         &m_cBlockModel.GetMaterial("led_13"),
         &m_cBlockModel.GetMaterial("led_14"),
         &m_cBlockModel.GetMaterial("led_15"),
         &m_cBlockModel.GetMaterial("led_16"),
         &m_cBlockModel.GetMaterial("led_17"),
         &m_cBlockModel.GetMaterial("led_18"),
         &m_cBlockModel.GetMaterial("led_19"),
         &m_cBlockModel.GetMaterial("led_20"),
         &m_cBlockModel.GetMaterial("led_21"),
         &m_cBlockModel.GetMaterial("led_22"),
         &m_cBlockModel.GetMaterial("led_23"),
      },
      /* initialize the tag texture */
      m_arrTagTexture {
         TAG_BLACK, TAG_BLACK, TAG_BLACK, TAG_BLACK, TAG_BLACK, TAG_BLACK, TAG_BLACK, TAG_BLACK,
         TAG_BLACK, TAG_BLACK, TAG_WHITE, TAG_WHITE, TAG_WHITE, TAG_BLACK, TAG_WHITE, TAG_BLACK,
         TAG_BLACK, TAG_BLACK, TAG_WHITE, TAG_WHITE, TAG_BLACK, TAG_BLACK, TAG_BLACK, TAG_BLACK,
         TAG_BLACK, TAG_BLACK, TAG_WHITE, TAG_BLACK, TAG_BLACK, TAG_BLACK, TAG_WHITE, TAG_BLACK,
         TAG_BLACK, TAG_WHITE, TAG_WHITE, TAG_WHITE, TAG_BLACK, TAG_WHITE, TAG_BLACK, TAG_BLACK,
         TAG_BLACK, TAG_BLACK, TAG_BLACK, TAG_WHITE, TAG_BLACK, TAG_BLACK, TAG_BLACK, TAG_BLACK,
         TAG_BLACK, TAG_WHITE, TAG_BLACK, TAG_BLACK, TAG_BLACK, TAG_WHITE, TAG_BLACK, TAG_BLACK,
         TAG_BLACK, TAG_BLACK, TAG_BLACK, TAG_BLACK, TAG_BLACK, TAG_BLACK, TAG_BLACK, TAG_BLACK,
      } {
      /* generate the tag texture */
      GLuint unTagTex;
      glGenTextures(1, &unTagTex);
      glBindTexture(GL_TEXTURE_2D, unTagTex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_FLOAT, m_arrTagTexture.data());
      /* draw normalized tag into list */
      m_unTagList = glGenLists(1);
      glNewList(m_unTagList, GL_COMPILE);
      glEnable(GL_NORMALIZE);
      glDisable(GL_LIGHTING);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, unTagTex);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glBegin(GL_QUADS);
      glNormal3f(0.0f, 0.0f, 1.0f);
      glTexCoord2f(1.0f, 1.0f); glVertex2f( 0.5f,  0.5f);
      glTexCoord2f(0.0f, 1.0f); glVertex2f(-0.5f,  0.5f);
      glTexCoord2f(0.0f, 0.0f); glVertex2f(-0.5f, -0.5f);
      glTexCoord2f(1.0f, 0.0f); glVertex2f( 0.5f, -0.5f);
      glEnd();
      glDisable(GL_TEXTURE_2D);
      glEnable(GL_LIGHTING);
      glDisable(GL_NORMALIZE);
      glEndList();
   }

   /****************************************/
   /****************************************/

   CQTOpenGLBlock::~CQTOpenGLBlock() {
      /* delete the tag drawing list */
      glDeleteLists(m_unTagList, 1);
   }

   /****************************************/
   /****************************************/

   void CQTOpenGLBlock::Draw(CBlockEntity& c_entity) {
      /* update the LED materials */
      const CDirectionalLEDEquippedEntity& c_leds = c_entity.GetDirectionalLEDEquippedEntity();
      for(UInt32 un_material_idx = 0;
          un_material_idx < m_arrLEDs.size();
          un_material_idx++) {
         const CColor& cColor = c_leds.GetLED(un_material_idx).GetColor();
         std::array<GLfloat, 4> arrColor = {
            cColor.GetRed() / 255.0f,
            cColor.GetGreen() / 255.0f,
            cColor.GetBlue() / 255.0f,
            1.0f
         };
         m_arrLEDs[un_material_idx]->Emission = arrColor;
      }
      /* draw the body */
      m_cBlockModel.Draw();
      /* draw tags */
      CRadians cZ, cY, cX;
      for(const CTagEquippedEntity::SInstance& s_instance :
             c_entity.GetTagEquippedEntity().GetInstances()) {
         Real fScaling = s_instance.Tag.GetSideLength();
         const CVector3& cTagPosition = s_instance.PositionOffset;
         const CQuaternion& cTagOrientation = s_instance.OrientationOffset;
         cTagOrientation.ToEulerAngles(cZ, cY, cX);
         glPushMatrix();
         glTranslatef(cTagPosition.GetX(),
                      cTagPosition.GetY(),
                      cTagPosition.GetZ());
         glRotatef(ToDegrees(cX).GetValue(), 1.0f, 0.0f, 0.0f);
         glRotatef(ToDegrees(cY).GetValue(), 0.0f, 1.0f, 0.0f);
         glRotatef(ToDegrees(cZ).GetValue(), 0.0f, 0.0f, 1.0f);
         glScalef(fScaling, fScaling, 1.0f);
         glCallList(m_unTagList);
         glPopMatrix();
      }
   }

   /****************************************/
   /****************************************/

   class CQTOpenGLOperationDrawBlockNormal : public CQTOpenGLOperationDrawNormal {
   public:
      void ApplyTo(CQTOpenGLWidget& c_visualization,
                   CBlockEntity& c_entity) {
         static CQTOpenGLBlock m_cModel;
         c_visualization.DrawEntity(c_entity.GetEmbodiedEntity());
         m_cModel.Draw(c_entity);
         c_visualization.DrawRays(c_entity.GetControllableEntity());
      }
   };

   /****************************************/
   /****************************************/

   class CQTOpenGLOperationDrawBlockSelected : public CQTOpenGLOperationDrawSelected {
   public:
      void ApplyTo(CQTOpenGLWidget& c_visualization,
                   CBlockEntity& c_entity) {
         c_visualization.DrawBoundingBox(c_entity.GetEmbodiedEntity());
      }
   };

   /****************************************/
   /****************************************/

   REGISTER_QTOPENGL_ENTITY_OPERATION(CQTOpenGLOperationDrawNormal, CQTOpenGLOperationDrawBlockNormal, CBlockEntity);

   REGISTER_QTOPENGL_ENTITY_OPERATION(CQTOpenGLOperationDrawSelected, CQTOpenGLOperationDrawBlockSelected, CBlockEntity);

   /****************************************/
   /****************************************/

}
