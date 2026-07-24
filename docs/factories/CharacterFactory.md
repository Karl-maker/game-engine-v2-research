# CharacterFactory

## Purpose

- CharacterFactory creates entities from data-driven config.

## Use when

- Creates a character entity with the expected gameplay and rig components.

## Config fields

- `meshReference` — Field of type `std::string` used by systems that consume this component.
- `leftHandKey` — Field of type `std::string` used by systems that consume this component.
- `rightHandKey` — Field of type `std::string` used by systems that consume this component.
- `headKey` — Field of type `std::string` used by systems that consume this component.

## Example JSON

```json
{
  "factory": "character",
  "config": {
    "name": "hero",
    "meshReference": "assets/models/business-man/scene.gltf",
    "leftHandKey": "left_hand",
    "rightHandKey": "right_hand",
    "headKey": "head",
    "positionLocal": [0.0, 0.0, 0.0],
    "rotationDeg": [0.0, 180.0, 0.0],
    "mass": 22.0
  }
}
```

## Notes

- This is the primary entry point for player, NPC, and humanoid actor spawns.
