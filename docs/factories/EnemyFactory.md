# EnemyFactory

## Purpose

- EnemyFactory creates entities from data-driven config.

## Use when

- Reserved for enemy-specific spawn behavior. The current scaffold keeps it as a separate key so enemy content can diverge later.

## Config fields

- This factory currently exposes a minimal config surface in the header.

## Example JSON

```json
{
  "factory": "enemy",
  "config": {
    "name": "grunt_01"
  }
}
```

## Notes

- If the factory is still a stub in your branch, use `character` or `entity` until you add enemy-specific setup.
