# M.I.L.O deterministic pattern memory

M.I.L.O V0.20.2 extends the V0.19 personality layer with a compact, Nyx-derived
component grammar. It does not use an LLM, network service, embeddings, or a
stored chat log. The same profile and input sequence always produce the same
state, route, and response.

## What is learned

M.I.L.O observes structural classes rather than vocabulary:

- question and exclamation punctuation;
- short input and uppercase tendency;
- courtesy/social lead-in followed by an action;
- validated action followed by an object;
- action phrased as a question;
- use of the natural request gateway;
- bounded operational spelling corrections; and
- the most recent abstract intent identifier.

This is deliberately analogous to generalized unigram/bigram learning. The
stored units are grammar classes such as `COURTESY`, `ACTION`, and `OBJECT`, not
the user's words. `traits` displays the counters. No sentence, filename, search
text, or phrase is written into the personality profile.

## Persistent format

The first observed interaction creates `MILO.MEM`, a single-cluster 24-byte
FAT12 file. It is validated at boot using its magic, format version, exact size,
and XOR checksum. Once present, updates rewrite only its one data sector.

| Offset | Size | Meaning |
| ---: | ---: | --- |
| 0 | 4 | ASCII magic `MILO` |
| 4 | 1 | Format version (`2`) |
| 5 | 1 | Interaction count |
| 6 | 1 | Warmth, capped at 100 |
| 7 | 1 | Cheek, capped at 100 |
| 8 | 1 | Curiosity, capped at 100 |
| 9 | 1 | Question-pattern count |
| 10 | 1 | Exclamation-pattern count |
| 11 | 1 | Short-input count |
| 12 | 1 | Uppercase-input count |
| 13 | 1 | Most recent speech-signal flags |
| 14 | 1 | Deterministic response cycle |
| 15 | 1 | Courtesy/social lead-in count |
| 16 | 1 | Validated natural-action count |
| 17 | 1 | Question-shaped action count |
| 18 | 1 | Corrected operational-token count |
| 19 | 1 | `ACTION -> OBJECT` pattern count |
| 20 | 1 | `SOCIAL -> ACTION` pattern count |
| 21 | 1 | Natural gateway route count |
| 22 | 1 | Most recent abstract intent ID |
| 23 | 1 | XOR checksum of bytes 0 through 22 |

Counters saturate at 255. Human-readable traits remain capped at 100. A valid
16-byte V0.19 profile is verified and migrated in memory; its next save replaces
the old file with format 2. An absent, malformed, or corrupt profile safely
falls back to compiled defaults.

## Privacy and determinism boundary

Routing uses a transient 64-byte normalized copy. The visible command/history
entry is unchanged, and the copy is never persisted. Fixed local response packs
are selected from structural state; learned state can influence selection but
cannot create new wording. There is no probabilistic model or hidden corpus.

V0.20.1 preserves the renderer's loop and entity registers, correcting the
V0.20 `traits` overrun and zero-byte directory display without changing the
profile format.
