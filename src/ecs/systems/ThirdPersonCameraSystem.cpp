#include "ecs/systems/ThirdPersonCameraSystem.h"

// Author: Karl-Johan Bailey

#include "ecs/components/ControllerComponent.h"
#include "ecs/components/MotionComponent.h"
#include "ecs/components/ThirdPersonCameraComponent.h"
#include "ecs/components/TransformComponent.h"
#include "math/Vec3.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kDegToRad = kPi / 180.0f;

// Character-relative shoulder side.
//  1.0f = camera on character's right.
// -1.0f = camera on character's left.
constexpr float kShoulderSide = -1.0f;
constexpr float kIdleOrbitSpeedThreshold = 0.05f;
constexpr float kIdleOrbitIntentThresholdSq = 0.0001f;

float lerp(
    float a,
    float b,
    float t) {

  return a + (b - a) * t;
}

math::Vec3 lerpVec3(
    const math::Vec3& a,
    const math::Vec3& b,
    float t) {

  return {
      lerp(a.x, b.x, t),
      lerp(a.y, b.y, t),
      lerp(a.z, b.z, t)};
}

bool hasMoveIntent(
    const ecs::ControllerComponent* controller) {

  if (!controller ||
      !controller->enabled) {
    return false;
  }

  if (controller->moveRequest.hasDirection &&
      math::lengthSq(
          controller->moveRequest.direction) >
          kIdleOrbitIntentThresholdSq) {
    return true;
  }

  if (controller->moveRequest.hasDestination) {
    return true;
  }

  return false;
}

}  // namespace

namespace ecs::systems {

void ThirdPersonCameraSystem::tick(
    EntityRegistry& registry,
    double deltaSeconds) const {

  const float dt =
      static_cast<float>(
          std::clamp(
              deltaSeconds,
              0.0,
              0.25));

  registry.view<
      ecs::ThirdPersonCameraComponent,
      ecs::TransformComponent>(
      [&](ecs::EntityId,
          ecs::ThirdPersonCameraComponent& cam,
          ecs::TransformComponent& camTr) {

        if (!cam.enabled ||
            cam.target ==
                ecs::kInvalidEntityId) {
          return;
        }

        auto* targetTr =
            registry.tryGet<
                ecs::TransformComponent>(
                cam.target);

        if (!targetTr) {
          return;
        }

        auto* motion =
            registry.tryGet<
                ecs::MotionComponent>(
                cam.target);

        // ---------------------------------------------------------
        // PLAYER LOOK
        //
        // Keep the existing control behavior.
        //
        // X = camera pitch
        // Y = character/camera yaw
        // ---------------------------------------------------------

        auto* controller =
            registry.tryGet<
                ecs::ControllerComponent>(
                cam.target);

        if (controller) {

          if (controller->enabled &&
              controller->lookRequest.hasRequest) {

            cam.yawDeg +=
                controller->
                    lookRequest.lookDelta.y;

            cam.pitchDeg =
                std::clamp(
                    cam.pitchDeg +
                        controller->
                    lookRequest.lookDelta.x,

                    cam.minPitchDeg,
                    cam.maxPitchDeg);
          }
        }

        // ---------------------------------------------------------
        // VELOCITY
        // ---------------------------------------------------------

        float currentSpeed = 0.0f;

        if (motion) {
          currentSpeed =
              motion->currentSpeed;
        }

        const bool movingWithIntent =
            currentSpeed >
                kIdleOrbitSpeedThreshold ||
            (motion &&
             motion->isMoving) ||
            hasMoveIntent(
                controller);

        if (movingWithIntent) {
          targetTr->rotation.y =
              cam.yawDeg;
        }

        const float orbitYawDeg =
            movingWithIntent
                ? targetTr->rotation.y
                : cam.yawDeg;

        // ---------------------------------------------------------
        // CHARACTER FORWARD
        //
        // The character's facing direction controls the horizontal
        // orbit position of the camera.
        // ---------------------------------------------------------

        const math::Vec3 characterForward{
            std::sin(
                orbitYawDeg *
                kDegToRad),

            0.0f,

            std::cos(
                orbitYawDeg *
                kDegToRad)
        };

        // ---------------------------------------------------------
        // CHARACTER RIGHT
        //
        // Used ONLY for the left/right shoulder offset.
        // ---------------------------------------------------------

        const math::Vec3 characterRight{
            std::cos(
                orbitYawDeg *
                kDegToRad),

            0.0f,

            -std::sin(
                orbitYawDeg *
                kDegToRad)
        };

        // ---------------------------------------------------------
        // CAMERA DISTANCE
        //
        // Faster movement pulls the camera farther back.
        // ---------------------------------------------------------

        float effectiveDistance =
            cam.distance;

        if (currentSpeed > 0.0f) {

          effectiveDistance =
              (currentSpeed * 0.2f) +
              1.7f;
        }

        // ---------------------------------------------------------
        // SHOULDER OFFSET
        //
        // Stationary:
        //   Camera stays strongly over the shoulder.
        //
        // Moving fast:
        //   Camera moves toward center behind character.
        // ---------------------------------------------------------

        const float maxShoulderOffset =
            0.8f;

        const float minShoulderOffset =
            0.0f;

        const float speedFactor =
            std::clamp(
                currentSpeed / 5.0f,
                0.0f,
                1.0f);

        const float shoulderOffsetAmount =
            lerp(
                maxShoulderOffset,
                minShoulderOffset,
                speedFactor);

        const math::Vec3 shoulderOffset =
            characterRight *
            (shoulderOffsetAmount *
             kShoulderSide);

        // ---------------------------------------------------------
        // CAMERA POSITION
        //
        // The camera is positioned behind the character's facing
        // direction and shifted horizontally toward the selected
        // shoulder.
        //
        // Pitch is NOT used here. This prevents looking up/down
        // from moving the camera vertically around the character.
        // ---------------------------------------------------------

        const math::Vec3 desired =
            targetTr->position +

            math::Vec3{
                0.0f,
                cam.targetOffset.y +
                    cam.height,
                0.0f} -

            characterForward *
                effectiveDistance +

            shoulderOffset;

        // ---------------------------------------------------------
        // CAMERA POSITION FOLLOW
        // ---------------------------------------------------------

        const float positionT =
            1.0f -
            std::exp(
                -cam.followSharpness *
                dt);

        camTr.position =
            lerpVec3(
                camTr.position,
                desired,
                positionT);

        // ---------------------------------------------------------
        // CAMERA ROTATION
        //
        // Horizontal direction follows the character.
        //
        // Vertical direction is controlled directly by cam.pitchDeg
        // so looking up/down works independently.
        // ---------------------------------------------------------

        camTr.rotation.x =
            cam.pitchDeg;

        camTr.rotation.y =
            orbitYawDeg;

        camTr.rotation.z =
            0.0f;

      });
}

}  // namespace ecs::systems
