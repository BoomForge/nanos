# Nyx-derived routing in M.I.L.O V0.20.2

V0.20.2 ports the small deterministic ideas from the old Nyx chat core rather
than its Go runtime, Windows integrations, or optional model layer.

## Turn boundary

1. Exact shell commands retain authority.
2. A private lowercase routing copy is made without changing visible input.
3. Courtesy and social lead-ins are parsed as reusable grammar components.
4. A known action is classified before its object is extracted.
5. Only the deterministic kernel performs the validated action.
6. Unclaimed text becomes ordinary local M.I.L.O conversation.

This permits compositional requests including:

- `please show me the files`
- `can you read README.TXT?`
- `show file NOTES.TXT`
- `find machinery in README.TXT`
- `search README.TXT for machinery`
- `tell me how much memory we have`

The parser contains operation vocabulary, not complete example sentences or
topic phrases. Filenames and search text are extracted as transient entities.

## Typo policy

Known operational tokens of four or more characters accept one insertion,
deletion, substitution, or adjacent transposition. Thus `reed`, `seach`, and
`lsit` can route as `read`, `search`, and `list`. Short tokens are exact-only to
avoid unsafe collisions.

Read-only `type` and `find` operations also try a conservative FAT12 filename
match after exact lookup fails. It accepts one substituted character or adjacent
swap in the padded 8.3 name and proceeds only when exactly one root entry
matches. Multiple candidates produce an ambiguity message.

Delete, rename, copy, write, and editor targets never receive fuzzy filename
correction. Destructive commands remain explicit shell syntax. The router will
not turn an uncertain spelling into a destructive action.

## Size boundary

The gateway, generic typo matcher, grammar tables, profile migration, and
read-only filename resolver all live inside the fixed 16 KiB kernel reservation.
No filesystem offsets or boot stages were expanded for V0.20.2.

V0.20.1 also gives natural and traditional `find` requests one parser. Both
`find FILE.TXT text` and `find text in FILE.TXT` therefore resolve identically.

V0.20.2 recognizes natural or misspelled file-changing intents without granting
them authority. Delete, rename, write, and copy near-misses state explicitly
that nothing changed and display the exact command required.
