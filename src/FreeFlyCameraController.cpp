#include "FreeFlyCameraController.h"

// TODO: input system
#include <windows.h>

namespace LambdaCore
{    
    static bool CheckKey(int key)
    {
        return (::GetAsyncKeyState(key) & 0x8000) > 0;
    }

    static glm::vec2 GetMousePos()
    {
        POINT p;
        ::GetCursorPos(&p);
        return glm::vec2(p.x, p.y);
    }

    FreeFlyCameraController::FreeFlyCameraController(const Commons::Render::CameraPtr& camera)
        : m_camera(camera)
    {
    }

    void FreeFlyCameraController::update(float delta)
    {
        static const float SPEED = 600.F;
        static const float ANG_SPEED = 1.5F;

        // Translation:
        glm::vec4 direction;

        if (CheckKey('W'))
        {
            direction += glm::vec4(0.F, 0.F, -1.F, 0.F);
        }		
        else if (CheckKey('S'))
        {
            direction += glm::vec4(0.F, 0.F, 1.F, 0.F);
        }

        if (CheckKey('A'))
        {
            direction += glm::vec4(-1.F, 0.F, 0.F, 0.F);
        }
        else if (CheckKey('D'))
        {
            direction += glm::vec4(1.F, 0.F, 0.F, 0.F);
        }

        if (CheckKey('Q'))
        {
            direction += glm::vec4(0.F, 1.F, 0.F, 0.F);
        }
        else if (CheckKey('E'))
        {
            direction += glm::vec4(0.F, -1.F, 0.F, 0.F);
        }
       
        glm::mat4 rotationMatrix(glm::mat4_cast(m_camera->getRotation()));
        //direction = rotationMatrix * direction;

        glm::vec3 translation = m_camera->getTranslation() + glm::vec3(direction) * SPEED * delta;
        m_camera->setTranslation(translation);

        // Rotation:
        // TODO: get current or store rotation and translation in controller?

        //if (CheckKey('M')) // Mouse update
        {
            glm::vec3 angleSpeed; // Yaw Pitch Roll

            if (CheckKey(VK_LEFT))
            {
                angleSpeed += glm::vec3(-1.F, 0.F, 0.F);
            }
            else if (CheckKey(VK_RIGHT))
            {
                angleSpeed += glm::vec3(1.F, 0.F, 0.F);
            }

            if (CheckKey(VK_UP))
            {
                angleSpeed += glm::vec3(0.F, 1.F, 0.F);
            }
            else if (CheckKey(VK_DOWN))
            {
                angleSpeed += glm::vec3(0.F, -1.F, 0.F);
            }
            
            m_angles.y += (angleSpeed.x * ANG_SPEED * delta); // Yaw
            m_angles.x += (angleSpeed.y * ANG_SPEED * delta); // Pitch
            m_angles.z = 0.F; // Roll
        }

        // Quat: pitch, yaw, roll
        glm::quat qYaw(m_angles.x, 0.F, 1.F, 0.F);
        glm::quat qPitch(m_angles.y, 1.F, 0.F, 0.F);
        glm::quat qRoll(m_angles.z, 0.F, 0.F, 1.F);

        // TODO

        //glm::quat rotation = /*qRoll * qPitch * */qYaw;
        glm::quat rotation(m_angles);
        m_camera->setRotation(rotation);
    }
}
