#include "ecs/systems/BuoyancySystem.h"

#include "ecs/components/MeshComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/RippleComponent.h"

#include <vector>
#include <cmath>
#include <algorithm>

namespace ecs::systems {

namespace {

constexpr float kWaterHeightTolerance = 2.0f;
constexpr char kWaterShader[] = "graphics/shaders/water";


// ---------------------------------------------------------
// Buoyancy tuning
// ---------------------------------------------------------

// Side-to-side rocking angle
constexpr float kMaxRoll = 8.0f;

// Keep forward/back rotation disabled
constexpr float kMaxPitch = 0.0f;


// Slow ocean swell speed
constexpr float kWaveSpeed = 0.45f;


// How heavy the object feels
constexpr float kWaterResponse = 2.0f;


// Vertical movement amount
constexpr float kBobAmount = 0.08f;


}


void BuoyancySystem::tick(
    EntityRegistry& registry,
    double deltaTime,
    double elapsedSeconds) const {


    const float dt =
        static_cast<float>(deltaTime);


    const float time =
        static_cast<float>(elapsedSeconds);



    struct WaterSurface {
        float height;
    };


    std::vector<WaterSurface> waterSurfaces;



    // ---------------------------------------------------------
    // Find water surfaces
    // ---------------------------------------------------------
    registry.view<TerrainComponent, TransformComponent, ShaderComponent>(
        [&](EntityId entity,
            TerrainComponent& terrain,
            TransformComponent& transform,
            ShaderComponent& shader) {


            if (shader.shader.key != kWaterShader) {
                return;
            }


            waterSurfaces.push_back({
                transform.position.y
            });

        });



    if (waterSurfaces.empty()) {
        return;
    }



    // ---------------------------------------------------------
    // Find floating objects
    // ---------------------------------------------------------
    registry.view<MeshComponent, TransformComponent>(
        [&](EntityId entity,
            MeshComponent& mesh,
            TransformComponent& transform) {


            bool floating = false;

            float waterHeight = 0.0f;



            for (const auto& water : waterSurfaces) {

                float distance =
                    water.height - transform.position.y;


                if (distance >= 0.0f &&
                    distance <= kWaterHeightTolerance) {

                    floating = true;
                    waterHeight = water.height;
                    break;
                }

            }



            if (!floating) {
                return;
            }



            // -------------------------------------------------
            // Ocean wave force
            // -------------------------------------------------

            float wave =
                std::sin(time * kWaveSpeed) * 0.75f +
                std::sin(time * (kWaveSpeed * 0.25f)) * 0.25f;



            // Convert wave into roll angle
            float targetRoll =
                wave * kMaxRoll;



            // Smooth water force response
            float smoothing =
                1.0f -
                std::exp(-kWaterResponse * dt);



            // -------------------------------------------------
            // Apply side-to-side rocking
            // -------------------------------------------------

            transform.rotation.z +=
                (targetRoll - transform.rotation.z)
                * smoothing;



            // Keep pitch neutral
            transform.rotation.x +=
                (kMaxPitch - transform.rotation.x)
                * smoothing;



            // Keep yaw neutral
            transform.rotation.y +=
                (0.0f - transform.rotation.y)
                * smoothing;



            // -------------------------------------------------
            // Floating height
            // -------------------------------------------------

            // transform.position.y =
            //     waterHeight +
            //     (wave * kBobAmount);



            // -------------------------------------------------
            // Add ripple component if missing
            // -------------------------------------------------

            if (true) return;
            if (!registry.has<RippleComponent>(entity)) {

                auto& rc = registry.emplace<RippleComponent>(entity);

                rc.radiusMeters = 18.0f;
                rc.lengthMeters = 1.0f;
                rc.widthMeters = 1.5f;

                rc.strength = 0.22f;
                rc.magnitude = 3.25f;

                rc.frequency = 11.5f;
                rc.speed = 3.1f;

                rc.falloffPower = 1.65f;
                rc.tiling = 0.28f;

                rc.direction = {1.0f, 0.10f};
                rc.driftSpeed = 0.24f;

                rc.foamBoost = 0.72f;

                rc.noiseScale = 0.22f;
                rc.noiseStrength = 0.55f;
                rc.noiseSpeed = 0.85f;

                rc.textureEnabled = true;

                rc.texture = {
                    true,
                    "assets/textures/water/ripples/ripple-01.jpg",
                    0
                };

                rc.enabled = true;
            }


        });

}


} // namespace ecs::systems