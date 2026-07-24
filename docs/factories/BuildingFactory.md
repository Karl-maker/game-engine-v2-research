# BuildingFactory

## Purpose

- BuildingFactory creates entities from data-driven config.

## Use when

- Reserved for building and structure content such as houses, walls, towers, and interactive set pieces.

## Config fields

- This factory currently exposes a minimal config surface in the header.

## Example JSON

```json
{
  "factory": "building",
  "config": {
    "name": "hut_01"
  }
}
```

## Notes

- Use this key once you want buildings to have their own spawn contract and maintenance logic.
