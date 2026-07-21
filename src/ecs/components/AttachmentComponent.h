#pragma once

// Author: Karl-Johan Bailey
//
// AttachmentComponent
// Describes how one entity is attached to (or driven by) another entity.
//
// Example idea:
// - Entity `main_character` has an AttachmentComponent that targets `camera_01`
// - A system interprets these descriptors each frame and updates transforms accordingly
//
// This component is intentionally descriptive only:
// - It stores references + settings.
// - It does NOT apply transforms by itself (that is system logic).

#include "ecs/EntityId.h"
#include "math/Vec3.h"

#include <string>
#include <vector>

namespace ecs {

struct AttachmentComponent {
  enum class Mode {
    Parent,   // Hard parent: child matches parent (optionally with offsets).
    Follow,   // Smoothly follow target position and/or rotation.
    Orbit,    // Orbit around target using yaw/pitch + radius.
    LookAt,   // Rotate to look at target (optionally with offsets).
    Socket,   // Attach to a named socket/bone on the target (string key).
    Spring,   // Spring-damped follow (e.g. camera boom).
  };

  enum class Space {
    Local,  // Offsets interpreted in target local space.
    World,  // Offsets interpreted in world space.
  };

  struct Attachment {
    // Which entity this attachment references.
    EntityId targetEntity = kInvalidEntityId;

    // How the attachment should be interpreted by the attachment system.
    Mode mode = Mode::Follow;
    bool enabled = true;

    // Offsets to apply when computing the attached transform.
    math::Vec3 positionOffset{0.0f, 0.0f, 0.0f};
    math::Vec3 rotationOffset{0.0f, 0.0f, 0.0f};  // pitch/yaw/roll in degrees
    math::Vec3 scaleOffset{0.0f, 0.0f, 0.0f};     // additive scale offset (system-defined)

    // What to inherit from the target.
    bool inheritPosition = true;
    bool inheritRotation = true;
    bool inheritScale = true;

    // Whether offsets are interpreted in local or world space.
    Space space = Space::Local;

    // General interpolation (smoothing). Systems may use one or more of these.
    float positionSpeed = 8.0f;
    float rotationSpeed = 12.0f;
    float interpolationSpeed = 10.0f;

    // Socket mode: a string key for a bone/socket/marker on the target.
    std::string socketKey;

    // Orbit mode (degrees for angles):
    float orbitRadius = 5.0f;
    float minPitchDeg = -45.0f;
    float maxPitchDeg = 70.0f;
    float currentYawDeg = 0.0f;
    float currentPitchDeg = 0.0f;
  };

  // An entity can have many attachments (camera, weapon, companions, etc).
  std::vector<Attachment> attachments;
};

}  // namespace ecs
