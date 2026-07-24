# WeaponFactory

## Purpose

- WeaponFactory creates entities from data-driven config.

## Use when

- Reserved for weapon-specific spawn behavior and inventory attachments.

## Config fields

- This factory currently exposes a minimal config surface in the header.

## Example JSON

```json
{
  "factory": "weapon",
  "config": {
    "name": "sword_01"
  }
}
```

## Notes

- This key exists so weapon content can eventually carry its own spawn rules instead of piggybacking on characters.
