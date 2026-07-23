#pragma once

// Author: Karl-Johan Bailey
//
// GrassPatchComponent (descriptive only)
// Describes GPU-instanced vegetation (primarily grass) for another system to generate/render.
//
// Design goals (demo-friendly, scalable):
// - No ECS entity per blade/clump; a renderer/system builds instance buffers.
// - Multiple layers/species to avoid obvious repetition.
// - Density/slope/altitude/noise rules for natural variation.
// - Wind + interaction are handled in shaders (driven by shared uniforms).

#include "ecs/EntityId.h"
#include "math/Vec3.h"
#include "terrain/NoiseConfig.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ecs {

struct GrassPatchComponent {
  struct GrassLayer final {
    // Engine-defined species key (renderer decides mesh/shader details for this species).
    // Suggested defaults: "ShortGrass", "TallGrass", "BroadLeafGrass", "DryGrass",
    // "Weed", "SmallFlower", "GroundCover".
    std::string species = "GroundCover";

    // Instances per square meter before masks/noise.
    float density = 5.0f;

    // Size range in meters (uniform scale applied to the clump mesh).
    float minScale = 0.55f;
    float maxScale = 0.95f;

    // Blade spacing multiplier inside each clump. Lower = tighter, denser tuft.
    float bladeSpacing = 0.2f;

    // Blade bend/curve controls.
    float bendStrength = 0.35f;
    float curveStrength = 0.18f;
    float twistStrength = 0.08f;

    // Terrain masks (optional; interpreted by the grass system).
    // Slope is in degrees (0 = flat).
    float minSlopeDeg = 0.0f;
    float maxSlopeDeg = 42.0f;
    float minAltitude = -10000.0f;
    float maxAltitude = 10000.0f;

    // Patchiness noise (world-space; higher strength = denser in high-noise regions).
    float noiseScale = 0.06f;
    float noiseStrength = 0.65f;  // 0..1

    // Wind contribution for this layer (shader-defined units).
    float windStrength = 1.0f;

    // Distance fade/cull (meters). Keep near grass dense; let terrain shader do the far field.
    float maxDistance = 30.0f;
  };

  bool enabled = true;

  // Patch area in XZ (x=width, z=depth), centered on the entity transform.
  math::Vec3 area{14.0f, 0.0f, 14.0f};

  // Base density multiplier applied to each layer's density.
  float densityMultiplier = 1.25f;

  std::uint32_t seed = 12345u;

  // Terrain-like density mask. High-density zones and empty zones come from this noise.
  terrain::NoiseConfig densityNoise{.type = terrain::NoiseType::Perlin, .seed = 12345u, .frequency = 0.03f, .octaves = 3,
                                    .lacunarity = 2.0f, .persistence = 0.55f};
  float densityNoiseThreshold = 0.42f;
  float densityNoiseContrast = 3.0f;
  float densityNoiseStrength = 1.0f;

  // Terrain selection (optional). If invalid, systems may use the first visible terrain.
  EntityId sourceTerrainEntity = kInvalidEntityId;

  // Layers (micro/hero/secondary vegetation). More layers = more variation.
  std::vector<GrassLayer> layers = {
      GrassLayer{.species = "GroundCover",
                 .density = 13.0f,
                 .minScale = 0.24f,
                 .maxScale = 0.50f,
                 .bladeSpacing = 0.12f,
                 .bendStrength = 0.26f,
                 .curveStrength = 0.20f,
                 .twistStrength = 0.10f,
                 .noiseScale = 0.075f,
                 .noiseStrength = 0.55f,
                 .windStrength = 0.75f,
                 .maxDistance = 20.0f},
      GrassLayer{.species = "TallGrass",
                 .density = 5.8f,
                 .minScale = 0.56f,
                 .maxScale = 1.05f,
                 .bladeSpacing = 0.16f,
                 .bendStrength = 0.42f,
                 .curveStrength = 0.28f,
                 .twistStrength = 0.12f,
                 .noiseScale = 0.050f,
                 .noiseStrength = 0.72f,
                 .windStrength = 1.0f,
                 .maxDistance = 34.0f},
      GrassLayer{.species = "BroadLeafGrass",
                 .density = 1.9f,
                 .minScale = 0.42f,
                 .maxScale = 0.82f,
                 .bladeSpacing = 0.18f,
                 .bendStrength = 0.34f,
                 .curveStrength = 0.24f,
                 .twistStrength = 0.10f,
                 .noiseScale = 0.060f,
                 .noiseStrength = 0.68f,
                 .windStrength = 0.85f,
                 .maxDistance = 27.0f},
  };

  // Interaction (player/actors) handled in shader as a set of influence spheres.
  bool interactionEnabled = true;
  float interactionRadiusMeters = 1.25f;
  float interactionStrength = 1.0f;

  // Render settings hints (engine-defined).
  bool castShadows = false;
  bool receiveShadows = true;
  float lodBias = 1.0f;
};

}  // namespace ecs
