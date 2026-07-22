// Pebbles layer (shader snippet)
//
// This file is intended to be included by other shaders via the renderer's
// tiny `#include "path"` preprocessor.

float hash12(vec2 p);
float valueNoise(vec2 p);
float fbm(vec2 p);
float saturate(float x);

// A lightweight "cellular-ish" pebble mask on the XZ plane.
// Returns:
// - mask: 0..1 pebble coverage
// - edge: 0..1 where 1 is near pebble edge (for grime/darkening)
// - height: 0..1 (pebble height field)
vec3 pebbleMask(vec2 worldXZ, float scale, float density) {
  // Scale up into "pebble cells".
  vec2 p = worldXZ * scale;
  vec2 cell = floor(p);
  vec2 f = fract(p);

  float bestDist = 10.0;
  float bestId = 0.0;
  vec2 bestD = vec2(0.0);

  // Search neighboring cells for closest center.
  for (int j = -1; j <= 1; ++j) {
    for (int i = -1; i <= 1; ++i) {
      vec2 c = cell + vec2(float(i), float(j));
      float rnd = hash12(c);
      // Random center and slight anisotropy.
      vec2 center = vec2(hash12(c + 3.1), hash12(c + 7.7));
      vec2 d = (vec2(float(i), float(j)) + center) - f;

      // Per-cell rotation + anisotropy to vary pebble shapes.
      float ang = hash12(c + 11.1) * 6.2831853;
      float ca = cos(ang);
      float sa = sin(ang);
      mat2 R = mat2(ca, -sa, sa, ca);
      float aniso = mix(0.65, 1.45, hash12(c + 5.5));
      vec2 d2 = R * d;
      d2 *= vec2(aniso, 1.0 / aniso);

      float dist = dot(d2, d2);
      if (dist < bestDist) {
        bestDist = dist;
        bestId = rnd;
        bestD = d2;
      }
    }
  }

  // Pebble radius varies a bit per cell.
  float r = mix(0.18, 0.42, bestId);
  // Density is treated as a probability of cell containing a pebble.
  float present = step(1.0 - density, bestId);

  float d = sqrt(bestDist);
  float core = 1.0 - smoothstep(r * 0.78, r, d);
  float mask = present * core;
  float edge = present * smoothstep(r * 0.55, r * 0.98, d);

  // Height: a rounded profile with slight randomness.
  float height = present * pow(saturate(core), mix(0.55, 1.35, bestId));

  return vec3(mask, edge, height);
}

float pebbleHeightAt(vec2 worldXZ, float scale, float density) {
  return pebbleMask(worldXZ, scale, density).z;
}

void applyPebbles(inout vec3 albedo,
                  inout float roughness,
                  inout vec3 normal,
                  vec3 worldPos,
                  vec3 viewDir,
                  vec3 pebbleColor,
                  float pebbleRoughness,
                  float pebbleScale,
                  float pebbleDensity,
                  float blend,
                  float normalStrength,
                  float heightStrength) {
  // Parallax occlusion (very small step count) to make pebbles feel like 3D clumps.
  vec2 xz = worldPos.xz;
  float vy = max(0.15, abs(viewDir.y));
  vec2 vdir = normalize(viewDir.xz + vec2(1e-4));
  float parallax = (0.10 * heightStrength) / vy;
  vec2 stepDir = vdir * parallax;

  vec2 p = xz;
  // 6-step search from top layer down.
  for (int i = 0; i < 6; ++i) {
    float t = float(i) / 5.0;
    vec2 q = xz - stepDir * t;
    float h = pebbleHeightAt(q, pebbleScale, pebbleDensity);
    if (h > (1.0 - t)) {
      p = q;
      break;
    }
  }

  vec3 m = pebbleMask(p, pebbleScale, pebbleDensity);
  float mask = m.x;
  float edge = m.y;
  float height = m.z;
  if (mask <= 0.0001) return;

  // Slight grime around edges.
  albedo *= 1.0 - edge * 0.06 * blend;

  // Blend albedo/roughness toward pebble properties.
  // Keep pebbles close to the underlying dirt so they don't read as black spots.
  vec3 targetColor = mix(albedo, pebbleColor, 0.65);
  albedo = mix(albedo, targetColor, mask * blend);
  roughness = mix(roughness, pebbleRoughness, mask * blend);

  // Height-based cavity darkening and subtle rim highlight to read as clumps.
  float cavity = (1.0 - height) * (0.12 * blend);
  albedo *= 1.0 - cavity;
  float rim = pow(1.0 - saturate(dot(normalize(viewDir), normal)), 3.5) * 0.10;
  albedo += rim * mask * blend;

  // Bump pebbles: build a pseudo height field normal from mask gradient.
  float eps = 0.08;
  float hx = pebbleHeightAt(p + vec2(eps, 0.0), pebbleScale, pebbleDensity);
  float hz = pebbleHeightAt(p + vec2(0.0, eps), pebbleScale, pebbleDensity);
  vec2 grad = vec2(hx - height, hz - height) / eps;
  vec3 bump = normalize(vec3(-grad.x * (2.2 * heightStrength), 1.0, -grad.y * (2.2 * heightStrength)));
  normal = normalize(mix(normal, bump, mask * blend * normalStrength));
}
