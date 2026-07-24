# EntityFactory

## Purpose

- EntityFactory creates entities from data-driven config.

## Use when

- Creates a generic entity with transform and optional rigidbody settings. This is the simplest spawn path for chunk content.

## Config fields

- `name` — A stable content or debug identifier.
- `position` — Spatial placement or relative offset.
- `rotationDeg` — Orientation or angle tuning.
- `scale` — Size, reach, or distance tuning.
- `hasRigidbody` — Field of type `bool` used by systems that consume this component.
- `mass` — Field of type `float` used by systems that consume this component.
- `useGravity` — Movement or force tuning.

## Example JSON

```json
{
  "factory": "entity",
  "config": {
    "name": "spawn_marker",
    "positionLocal": [0.0, 0.0, 0.0],
    "rotationDeg": [0.0, 90.0, 0.0],
    "scale": [1.0, 1.0, 1.0],
    "rigidbody": {
      "mass": 22.0,
      "useGravity": true
    }
  }
}
```

## Notes

- Use this when you just need an entity shell with spatial data or a lightweight physics body.
