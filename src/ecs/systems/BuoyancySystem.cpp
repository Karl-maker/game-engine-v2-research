#include "ecs/systems/BuoyancySystem.h"

#include "ecs/components/MeshComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/RippleComponent.h"

#include <vector>
#include <cmath>
#include <cstdint>
#include <unordered_map>

namespace ecs::systems {

namespace {

constexpr float kWaterHeightTolerance = 2.0f;
constexpr char kWaterShader[] = "graphics/shaders/water";


// ---------------------------------------------------------
// Buoyancy tuning
// ---------------------------------------------------------

// Very subtle water movement
constexpr float kPitchAmount = 0.35f;
constexpr float kYawAmount   = 0.15f;
constexpr float kRollAmount  = 5.0f;


// Slow ocean movement
constexpr float kWaveSpeed = 0.35f;


// Smoothness
constexpr float kWaterResponse = 2.0f;


// Object variation
constexpr float kSpeedVariation = 0.25f;
constexpr float kStrengthVariation = 0.35f;


constexpr float kTwoPi = 6.283185f;



struct BuoyancyOffset {
    float basePitch;
    float baseYaw;
    float baseRoll;

    float phase;
    float speed;
    float strength;
};


std::unordered_map<EntityId, BuoyancyOffset> s_offsets;



float random01(uint32_t value)
{
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;

    return static_cast<float>(value % 10000) / 10000.0f;
}


}


void BuoyancySystem::tick(
    EntityRegistry& registry,
    double deltaTime,
    double elapsedSeconds) const {


    const float dt =
        static_cast<float>(deltaTime);


    const float time =
        static_cast<float>(elapsedSeconds);



    float waterHeight = 0.0f;
    bool hasWater = false;



    // ---------------------------------------------------------
    // Find water
    // ---------------------------------------------------------

    registry.view<TerrainComponent,
                 TransformComponent,
                 ShaderComponent>(
        [&](EntityId entity,
            TerrainComponent& terrain,
            TransformComponent& transform,
            ShaderComponent& shader) {


            if (shader.shader.key != kWaterShader) {
                return;
            }


            waterHeight = transform.position.y;
            hasWater = true;

        });



    if (!hasWater) {
        return;
    }



    const float smoothing =
        1.0f -
        std::exp(-kWaterResponse * dt);




    // ---------------------------------------------------------
    // Floating objects
    // ---------------------------------------------------------

    registry.view<MeshComponent, TransformComponent>(
        [&](EntityId entity,
            MeshComponent& mesh,
            TransformComponent& transform) {


            float distance =
                waterHeight -
                transform.position.y;



            if (distance < 0.0f ||
                distance > kWaterHeightTolerance) {

                return;
            }



            // -------------------------------------------------
            // Create unique buoyancy settings once
            // -------------------------------------------------

            auto it =
                s_offsets.find(entity);



            if (it == s_offsets.end()) {

                uint32_t seed =
                    static_cast<uint32_t>(entity);


                s_offsets[entity] = {

                    transform.rotation.x,
                    transform.rotation.y,
                    transform.rotation.z,

                    random01(seed) * kTwoPi,

                    1.0f +
                    ((random01(seed + 10) - 0.5f)
                     *
                     kSpeedVariation),

                    1.0f +
                    ((random01(seed + 20) - 0.5f)
                     *
                     kStrengthVariation)
                };


                it =
                    s_offsets.find(entity);
            }



            auto& buoyancy =
                it->second;




            // -------------------------------------------------
            // Water movement
            // -------------------------------------------------

            float pitchWave =
                std::sin(
                    time *
                    kWaveSpeed *
                    buoyancy.speed
                    +
                    buoyancy.phase
                );


            float yawWave =
                std::sin(
                    time *
                    kWaveSpeed *
                    0.5f *
                    buoyancy.speed
                    +
                    buoyancy.phase * 2.0f
                );


            float rollWave =
                std::sin(
                    time *
                    kWaveSpeed *
                    buoyancy.speed
                    +
                    buoyancy.phase * 1.3f
                );




            // -------------------------------------------------
            // Target rotation
            // Base rotation + water offset
            // -------------------------------------------------

            float targetPitch =
                buoyancy.basePitch +
                pitchWave *
                kPitchAmount *
                buoyancy.strength;


            float targetYaw =
                buoyancy.baseYaw +
                yawWave *
                kYawAmount *
                buoyancy.strength;


            float targetRoll =
                buoyancy.baseRoll +
                rollWave *
                kRollAmount *
                buoyancy.strength;




            // -------------------------------------------------
            // Smooth movement
            // -------------------------------------------------

            transform.rotation.x +=
                (targetPitch -
                 transform.rotation.x)
                 *
                 smoothing;


            transform.rotation.y +=
                (targetYaw -
                 transform.rotation.y)
                 *
                 smoothing;


            transform.rotation.z +=
                (targetRoll -
                 transform.rotation.z)
                 *
                 smoothing;


        });

}


} // namespace ecs::systems