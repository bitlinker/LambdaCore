#pragma once
#include <stdint.h>
#include <Render/RenderCommon.h>

namespace LambdaCore
{
    class Player
    {
    public:
        glm::vec3 mPos;
        glm::vec3 mRot;
        bool mIsDucking;
        bool mIsRunning;
        // TODO: weapons, HP, ...
    };

    class PlayerController
    {
    public:
        PlayerController();

    private:
        Player mPlayer;

        float mMaxSpeed;   // Default = 320
        float mAccelerate; // Default = 10
        float mAirAccelerate; // Default = 10
        float mMaxVelocity; /// Default = 2000;
    };
}