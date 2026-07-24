# EnemyFactory

## What it is

- EnemyFactory turns chunk JSON into live ECS entities.

## When to use it

- Use this for enemies once you want spawn rules that differ from generic characters.

## How it fits

- Chunk loading reads the JSON entry, picks a factory key, and passes the `config` object to the matching `IEntityFactory`.
- The factory owns entity creation details so chunk content can stay declarative.

## Config fields

- The config is currently minimal in the header, so the JSON contract is mostly defined in the factory implementation.

## Example JSON

```json
{
  "factory": "enemy",
  "config": {
    "name": "grunt_01"
  }
}
```

## Adding a new field

- Add the setting to `EnemyConfig` first.
- Parse the JSON in the factory implementation.
- Add an example chunk entry so future content authors can copy the pattern.
- Update `FactoryKeyService` if the new factory needs a new key.

## Notes

- If the branch still treats this as a placeholder, keep using `character` until the enemy contract is finished.
