# AssetFactory

## Purpose

- AssetFactory creates entities from data-driven config.

## Use when

- Reserved for non-character scene assets or reusable authored content that should still be created through the factory pipeline.

## Config fields

- This factory currently exposes a minimal config surface in the header.

## Example JSON

```json
{
  "factory": "asset",
  "config": {
    "name": "prop_statue"
  }
}
```

## Notes

- This is useful for props, markers, and authored objects that should participate in chunk loading but not gameplay rules.
