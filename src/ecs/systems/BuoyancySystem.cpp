#include "ecs/systems/BuoyancySystem.h"

#include "ecs/components/MeshComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/TerrainComponent.h"
#include "ecs/components/ShaderComponent.h"
#include "ecs/components/RippleComponent.h"
#include "ecs/components/RigidbodyComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/BuoyantComponent.h"
#include "math/Vec2.h"

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

// Ripple threshold - how close to target height to create ripples
constexpr float kRippleProximityThreshold = 0.1f;

struct BuoyancyOffset {
    float basePitch;
    float baseYaw;
    float baseRoll;
    float phase;
    float speed;
    float strength;
};

std::unordered_map<EntityId, BuoyancyOffset> s_offsets;

float random01(uint32_t value) {
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return static_cast<float>(value % 10000) / 10000.0f;
}

} // namespace

void BuoyancySystem::tick(
    EntityRegistry& registry,
    double deltaTime,
    double elapsedSeconds) const {

    const float dt = static_cast<float>(deltaTime);
    const float time = static_cast<float>(elapsedSeconds);

    float waterHeight = 0.0f;
    bool hasWater = false;

    // ---------------------------------------------------------
    // Find water surface
    // ---------------------------------------------------------
    registry.view<TerrainComponent, TransformComponent, ShaderComponent>(
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

    float rotationSmooth = 1.0f - std::exp(-kWaterResponse * dt);

    // ---------------------------------------------------------
    // Buoyant objects - using BuoyantComponent
    // ---------------------------------------------------------
    registry.view<TransformComponent, BuoyantComponent>(
        [&](EntityId entity,
            TransformComponent& transform,
            BuoyantComponent& buoyant) {

            // Not buoyant, ignore
            if (!buoyant.buoyant) {
                // Remove ripples if they exist
                if (registry.has<RippleComponent>(entity)) {
                    registry.remove<RippleComponent>(entity);
                }
                return;
            }

            // Calculate target floating height
            float targetHeight = waterHeight + buoyant.buoyancyHeight;

            // Get current position
            float currentY = transform.position.y;

            // Check if below target height (needs buoyancy lift)
            bool belowTarget = currentY < targetHeight;

            // Get optional components
            MotionComponent* motion = registry.tryGet<MotionComponent>(entity);
            RigidbodyComponent* rigidbody = registry.tryGet<RigidbodyComponent>(entity);

            // -------------------------------------------------
            // Buoyancy force - push toward target height
            // -------------------------------------------------
            if (belowTarget) {
                float heightDifference = targetHeight - currentY;

                if (motion != nullptr) {
                    // Use motion component - set velocity
                    motion->isGrounded = true;
                    float liftVelocity = heightDifference * kBuoyancyForce;
                    motion->velocity.y = liftVelocity;
                } else if (rigidbody != nullptr) {
                    // Use rigidbody - apply force
                    float lift = heightDifference * kBuoyancyForce * dt;
                    transform.position.y += lift;
                    if (rigidbody->linearVelocity.y < 0.0f) {
                        rigidbody->linearVelocity.y = 0.0f;
                    }
                } else {
                    // Direct position modification
                    float lift = heightDifference * kBuoyancyForce * dt;
                    transform.position.y += lift;
                }
            }

            // -------------------------------------------------
            // Sway - only if enabled
            // -------------------------------------------------
            if (buoyant.sway) {
                auto it = s_offsets.find(entity);

                if (it == s_offsets.end()) {
                    uint32_t seed = static_cast<uint32_t>(entity);

                    s_offsets[entity] = {
                        transform.rotation.x,
                        transform.rotation.y,
                        transform.rotation.z,
                        random01(seed) * kTwoPi,
                        1.0f + ((random01(seed + 10) - 0.5f) * kSpeedVariation),
                        1.0f + ((random01(seed + 20) - 0.5f) * kStrengthVariation)
                    };

                    it = s_offsets.find(entity);
                }

                auto& offset = it->second;

                // Calculate sway waves
                float pitchWave = std::sin(time * kWaveSpeed * offset.speed + offset.phase);
                float yawWave = std::sin(time * kWaveSpeed * 0.5f * offset.speed + offset.phase * 2.0f);
                float rollWave = std::sin(time * kWaveSpeed * offset.speed + offset.phase * 1.3f);

                float targetPitch = offset.basePitch + pitchWave * kPitchAmount * offset.strength;
                float targetYaw = offset.baseYaw + yawWave * kYawAmount * offset.strength;
                float targetRoll = offset.baseRoll + rollWave * kRollAmount * offset.strength;

                // Apply water rotation smoothly
                transform.rotation.x += (targetPitch - transform.rotation.x) * rotationSmooth;
                transform.rotation.y += (targetYaw - transform.rotation.y) * rotationSmooth;
                transform.rotation.z += (targetRoll - transform.rotation.z) * rotationSmooth;
            } else {
                // Remove sway offsets if sway is disabled
                s_offsets.erase(entity);
            }

            // -------------------------------------------------
            // Ripples - only when at buoyancy height (within 0.1 range)
            // -------------------------------------------------
            float distanceToTarget = std::abs(currentY - targetHeight);
            bool atBuoyancyHeight = distanceToTarget < kRippleProximityThreshold;

            if (atBuoyancyHeight) {
                // Determine if object is moving
                bool isMoving = false;
                float speed = 0.0f;

                if (motion != nullptr) {
                    float hSpeed = std::sqrt(
                        motion->velocity.x * motion->velocity.x +
                        motion->velocity.z * motion->velocity.z
                    );
                    speed = hSpeed;
                    isMoving = hSpeed > 0.15f;
                } else if (rigidbody != nullptr) {
                    float hSpeed = std::sqrt(
                        rigidbody->linearVelocity.x * rigidbody->linearVelocity.x +
                        rigidbody->linearVelocity.z * rigidbody->linearVelocity.z
                    );
                    speed = hSpeed;
                    isMoving = hSpeed > 0.15f;
                }

                if (!registry.has<RippleComponent>(entity)) {
                    // Create ripple component
                    auto& rc = registry.emplace<RippleComponent>(entity);

                    if (isMoving) {
                        // MOVING: Elongated wake with trail
                        rc.radiusMeters = 22.0f;
                        rc.lengthMeters = 4.5f;
                        rc.widthMeters = 2.0f;
                        rc.strength = 1.35f;
                        rc.magnitude = 5.5f;
                        rc.frequency = 8.5f;
                        rc.speed = 2.8f;
                        rc.falloffPower = 1.45f;
                        rc.tiling = 0.32f;

                        // Direction based on velocity
                        math::Vec2 moveDir;
                        if (motion != nullptr) {
                            moveDir = {motion->velocity.x, motion->velocity.z};
                        } else if (rigidbody != nullptr) {
                            moveDir = {rigidbody->linearVelocity.x, rigidbody->linearVelocity.z};
                        } else {
                            moveDir = {1.0f, 0.0f};
                        }
                        
                        float dirLen = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
                        if (dirLen > 0.01f) {
                            rc.direction = {moveDir.x / dirLen, moveDir.y / dirLen};
                        } else {
                            rc.direction = {1.0f, 0.10f};
                        }

                        rc.driftSpeed = 0.35f;
                        rc.foamBoost = 0.08f;
                        rc.noiseScale = 0.18f;
                        rc.noiseStrength = 0.15f;
                        rc.noiseSpeed = 0.95f;
                    } else {
                        // STATIONARY: Circular pulse rings
                        rc.radiusMeters = 12.0f;
                        rc.lengthMeters = 1.2f;
                        rc.widthMeters = 1.2f;
                        rc.strength = 0.85f;
                        rc.magnitude = 2.8f;
                        rc.frequency = 14.0f;
                        rc.speed = 3.8f;
                        rc.falloffPower = 2.2f;
                        rc.tiling = 0.22f;
                        rc.direction = {1.0f, 0.0f};
                        rc.driftSpeed = 0.12f;
                        rc.foamBoost = 0.01f;
                        rc.noiseScale = 0.08f;
                        rc.noiseStrength = 0.03f;
                        rc.noiseSpeed = 0.5f;
                    }

                    rc.textureEnabled = true;
                    rc.texture = {
                        true,
                        "assets/textures/water/ripples/ripple-01.jpg",
                        0
                    };
                    rc.enabled = true;
                } else {
                    // Update existing ripple component
                    auto& rc = registry.get<RippleComponent>(entity);
                    rc.enabled = true;

                    if (isMoving) {
                        float transitionSpeed = 3.0f * dt;

                        rc.lengthMeters += (4.5f - rc.lengthMeters) * transitionSpeed;
                        rc.widthMeters += (2.0f - rc.widthMeters) * transitionSpeed;
                        rc.strength += (1.35f - rc.strength) * transitionSpeed;
                        rc.magnitude += (5.5f - rc.magnitude) * transitionSpeed;
                        rc.frequency += (8.5f - rc.frequency) * transitionSpeed;
                        rc.speed += (2.8f - rc.speed) * transitionSpeed;
                        rc.falloffPower += (1.45f - rc.falloffPower) * transitionSpeed;
                        rc.foamBoost += (0.08f - rc.foamBoost) * transitionSpeed;
                        rc.noiseScale += (0.18f - rc.noiseScale) * transitionSpeed;
                        rc.noiseStrength += (0.15f - rc.noiseStrength) * transitionSpeed;
                        rc.noiseSpeed += (0.95f - rc.noiseSpeed) * transitionSpeed;
                        rc.driftSpeed += (0.35f - rc.driftSpeed) * transitionSpeed;

                        // Update direction
                        math::Vec2 moveDir;
                        if (motion != nullptr) {
                            moveDir = {motion->velocity.x, motion->velocity.z};
                        } else if (rigidbody != nullptr) {
                            moveDir = {rigidbody->linearVelocity.x, rigidbody->linearVelocity.z};
                        } else {
                            moveDir = {1.0f, 0.0f};
                        }

                        float dirLen = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
                        if (dirLen > 0.1f) {
                            math::Vec2 targetDir = {moveDir.x / dirLen, moveDir.y / dirLen};
                            rc.direction.x += (targetDir.x - rc.direction.x) * transitionSpeed;
                            rc.direction.y += (targetDir.y - rc.direction.y) * transitionSpeed;
                        }

                        float speedFactor = std::min(speed / 5.0f, 1.5f);
                        rc.strength *= (0.8f + speedFactor * 0.4f);
                        rc.magnitude *= (0.7f + speedFactor * 0.6f);
                    } else {
                        float transitionSpeed = 3.0f * dt;

                        rc.lengthMeters += (1.2f - rc.lengthMeters) * transitionSpeed;
                        rc.widthMeters += (1.2f - rc.widthMeters) * transitionSpeed;
                        rc.strength += (0.85f - rc.strength) * transitionSpeed;
                        rc.magnitude += (2.8f - rc.magnitude) * transitionSpeed;
                        rc.frequency += (14.0f - rc.frequency) * transitionSpeed;
                        rc.speed += (3.8f - rc.speed) * transitionSpeed;
                        rc.falloffPower += (2.2f - rc.falloffPower) * transitionSpeed;
                        rc.foamBoost += (0.01f - rc.foamBoost) * transitionSpeed;
                        rc.noiseScale += (0.08f - rc.noiseScale) * transitionSpeed;
                        rc.noiseStrength += (0.03f - rc.noiseStrength) * transitionSpeed;
                        rc.noiseSpeed += (0.5f - rc.noiseSpeed) * transitionSpeed;
                        rc.driftSpeed += (0.12f - rc.driftSpeed) * transitionSpeed;
                    }
                }
            } else {
                // Not at buoyancy height - remove ripples
                if (registry.has<RippleComponent>(entity)) {
                    registry.remove<RippleComponent>(entity);
                }
            }
        });
}

} // namespace ecs::systems