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
    // Suggested defaults: "BillboardGrassPlanes", "GroundCover", "TallGrass",
    // "BroadLeafGrass", "DryGrass", "Weed", "SmallFlower".
    std::string species = "BillboardGrassPlanes";

    // Describes the intended look; rendering remains shader-driven so the carpet
    // can look dense without needing huge geometry counts.
    std::string description = "Three intersecting billboard grass planes with texture-driven alpha and distance LOD.";

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

    // Legacy per-layer wind strength. BillboardGrassPlanes ignores this and uses
    // a tiny built-in GPU sway so the CPU path stays simple.
    float windStrength = 0.0f;

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

  // Island mask for shaping the grass patch itself. Use frequency/octaves/persistence/lacunarity
  // for the noise shape, then threshold/softness/contrast to cut it into controllable islands.
  terrain::NoiseConfig islandNoise{.type = terrain::NoiseType::Perlin, .seed = 4242u, .frequency = 0.045f, .octaves = 3,
                                   .lacunarity = 2.0f, .persistence = 0.58f};
  math::Vec3 islandNoiseOffset{0.0f, 0.0f, 0.0f};
  float islandNoiseThreshold = 0.44f;
  float islandNoiseSoftness = 0.18f;
  float islandNoiseContrast = 1.2f;
  float islandNoiseStrength = 1.0f;

  // Terrain selection (optional). If invalid, systems may use the first visible terrain.
  EntityId sourceTerrainEntity = kInvalidEntityId;

  // Layers. Default is intentionally one shader-driven carpet layer; the shader
  // handles haze, color bands, and fake density instead of stacking geometry layers.
  std::vector<GrassLayer> layers = {
      GrassLayer{.species = "BillboardGrassPlanes",
                 .density = 5.6f,
                 .minScale = 0.54f,
                 .maxScale = 0.92f,
                 .bladeSpacing = 0.56f,
                 .bendStrength = 0.18f,
                 .curveStrength = 0.22f,
                 .twistStrength = 0.10f,
                 .noiseScale = 0.055f,
                 .noiseStrength = 0.54f,
                 .windStrength = 0.0f,
                 .maxDistance = 34.0f},
  };

  // Interaction (player/actors) handled in shader as a set of influence spheres.
  bool interactionEnabled = false;
  float interactionRadiusMeters = 1.25f;
  float interactionStrength = 1.0f;

  // Render settings hints (engine-defined).
  bool castShadows = false;
  bool receiveShadows = true;
  float lodBias = 1.0f;
};

}  // namespace ecs
