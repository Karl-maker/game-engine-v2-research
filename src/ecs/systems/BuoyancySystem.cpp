#include "ecs/systems/BuoyancySystem.h"

#include "ecs/components/MeshComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/RippleComponent.h"
#include "ecs/components/RigidbodyComponent.h"
#include "ecs/components/MotionComponent.h"

#include <cmath>
#include <unordered_map>
#include <cstdint>


namespace ecs::systems {

namespace {

constexpr float kWaterHeightTolerance = 2.0f;
constexpr char kWaterShader[] = "graphics/shaders/water";


// ---------------------------------------------------------
// Buoyancy tuning
// ---------------------------------------------------------

// How strong objects are pushed upward
constexpr float kBuoyancyForce = 4.0f;


// Maximum water sway
constexpr float kPitchAmount = 0.35f;
constexpr float kYawAmount = 0.15f;
constexpr float kRollAmount = 5.0f;


// Ocean speed
constexpr float kWaveSpeed = 0.35f;


// Rotation smoothing
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
    // Find water surface
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


            waterHeight =
                transform.position.y;


            hasWater = true;

        });



    if (!hasWater) {
        return;
    }



    float rotationSmooth =
        1.0f -
        std::exp(-kWaterResponse * dt);




    // ---------------------------------------------------------
    // Buoyant objects
    // ---------------------------------------------------------

    registry.view<MeshComponent,
                 TransformComponent,
                 RigidbodyComponent>(
        [&](EntityId entity,
            MeshComponent& mesh,
            TransformComponent& transform,
            RigidbodyComponent& rigidbody) {



            // Not buoyant, ignore completely
            if (!rigidbody.buoyant) {
                return;
            }



            // -------------------------------------------------
            // Calculate desired floating height first - this is
            // the actual line buoyancy should engage/disengage
            // around, NOT the raw water surface. buoyancyHeight
            // shifts that line up (rests above surface, e.g. a
            // boat hull) or down (rests partially submerged), so
            // gating on waterHeight directly caused objects to
            // freeze right at sea level instead of ever reaching
            // targetHeight.
            // -------------------------------------------------

            float targetHeight =
                waterHeight +
                rigidbody.buoyancyHeight;



            bool belowTarget =
                transform.position.y < targetHeight;


            if (!belowTarget) {
                // At or above the intended floating height - leave
                // gravity/movement completely alone so items above
                // the water (or resting exactly at their float
                // line) fall/behave normally instead of buoyancy
                // holding them prematurely.
                return;
            }



            // Optional - not every buoyant entity will have one,
            // so always guard against it being absent below.
            MotionComponent* motion =
                registry.tryGet<MotionComponent>(entity);



            float heightDifference =
                targetHeight -
                transform.position.y;



            // -------------------------------------------------
            // Gentle upward correction
            // -------------------------------------------------

            if (motion != nullptr) {

                // MovementSystem is the single source of truth for turning
                // MotionComponent::velocity into transform.position
                // (tr.position = tr.position + motion.velocity * dt).
                // Buoyancy must only set velocity here - never touch
                // transform.position directly - or the two systems double-
                // apply movement and fight/cancel each other out.

                // Still below the float line - keep gravity off this
                // entity (GravitySystem skips anything with isGrounded
                // true) and push it upward via velocity.
                motion->isGrounded = true;

                float liftVelocity =
                    heightDifference *
                    kBuoyancyForce;


                motion->velocity.y = liftVelocity;

            } else {

                // No MotionComponent - fall back to writing position
                // directly, since nothing else will integrate this entity.
                float lift =
                    heightDifference *
                    kBuoyancyForce *
                    dt;


                transform.position.y += lift;


                if (rigidbody.linearVelocity.y < 0.0f) {
                    rigidbody.linearVelocity.y = 0.0f;
                }
            }

            // -------------------------------------------------
            // Create unique water movement
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


                    random01(seed) *
                    kTwoPi,


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



            auto& offset =
                it->second;




            // -------------------------------------------------
            // Water sway
            // -------------------------------------------------

            float pitchWave =
                std::sin(
                    time *
                    kWaveSpeed *
                    offset.speed
                    +
                    offset.phase
                );


            float yawWave =
                std::sin(
                    time *
                    kWaveSpeed *
                    0.5f *
                    offset.speed
                    +
                    offset.phase * 2.0f
                );


            float rollWave =
                std::sin(
                    time *
                    kWaveSpeed *
                    offset.speed
                    +
                    offset.phase * 1.3f
                );




            float targetPitch =
                offset.basePitch +
                pitchWave *
                kPitchAmount *
                offset.strength;



            float targetYaw =
                offset.baseYaw +
                yawWave *
                kYawAmount *
                offset.strength;



            float targetRoll =
                offset.baseRoll +
                rollWave *
                kRollAmount *
                offset.strength;




            // -------------------------------------------------
            // Apply water rotation
            // -------------------------------------------------

            transform.rotation.x +=
                (targetPitch -
                 transform.rotation.x)
                *
                rotationSmooth;


            transform.rotation.y +=
                (targetYaw -
                 transform.rotation.y)
                *
                rotationSmooth;


            transform.rotation.z +=
                (targetRoll -
                 transform.rotation.z)
                *
                rotationSmooth;


        });


}


} // namespace ecs::systems