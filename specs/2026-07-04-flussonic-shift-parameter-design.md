# Flussonic `shift` query parameter — Design

**Date:** 2026-07-04
**Status:** Approved, pending implementation plan
**Scope:** Add Flussonic-compatible `shift` query parameter to MistServer playback URLs.

## Goal

Allow MistServer to accept Flussonic-style playback URLs such as:

```
https://host/stream/index.m3u8?shift=100
```

where `shift=N` means "start playback N seconds behind the live edge." This is
exactly equivalent to MistServer's existing `startunix=-N`.

Example real-world URL this must support:

```
.../wfaadt.m3u8?bandwidth=2200&shift=0
```

Only `shift` is in scope. `bandwidth`, Flussonic archive path forms
(`/stream/archive-<from>-<duration>.m3u8`, `timeshift_abs-<utc>.m3u8`), and
absolute-time query forms are explicitly **out of scope** for this change.
Unknown query params like `bandwidth` are ignored, as they are today.

## Background / existing behavior

MistServer already supports a `startunix` query parameter. The relevant fact
that makes `shift` a thin alias:

- `startunix` is read at the single query-capture point in
  `HTTPOutput::onHTTP()` via the `HTTP_CONVERT` macro
  (`src/output/output_http.cpp`, ~line 417–433) into the base-class
  `targetParams` map. From there it is consumed uniformly by all HTTP outputs
  (HLS, CMAF, TS).
- In `src/output/output.cpp` (~line 1337–1357) and
  `src/output/output_cmaf.cpp` (`getRequestedWindowMs`, ~line 250–282),
  `startunix` (seconds) is multiplied to ms and, crucially, **values ≤
  36,000,000 ms (10 hours) are treated as relative to now**:

  ```cpp
  int64_t startUnix = atoll(targetParams["startunix"].c_str()) * 1000;
  if (startUnix <= 36000000){ startUnix += Util::unixMS(); } // relative
  targetParams["start"] = JSON::Value(startUnix - zUTC).asString();
  ```

  Therefore `startunix=-100` already means "100 seconds before live." Seek
  bounds are clamped later (~output.cpp:1382–1387), so a shift larger than the
  available DVR buffer safely clamps to the earliest available media.

## Design

### Behavior

| Input | Result |
|-------|--------|
| `shift=N`, N > 0 | Seek to N seconds before live, then play forward. Implemented as `startunix = -N`. |
| `shift=0` | No shift; normal live edge. |
| `shift` absent / non-numeric / ≤ 0 | No shift; normal live edge. |
| `shift` present **and** explicit `startunix` or `start` also present | Explicit `startunix`/`start` wins; `shift` is ignored. |
| `shift` beyond available DVR buffer | Existing seek-bounds clamping applies (clamps to earliest available). No new logic. |
| `shift=300&duration=60` | Works as a free side effect: `duration` already flows independently, giving a 60 s window starting 5 min ago. Not blocked. |

### Implementation point (Approach A)

Add the translation immediately after the `HTTP_CONVERT` block in
`HTTPOutput::onHTTP()` in `src/output/output_http.cpp` (~line 433). This is the
single location where all query params are captured into `targetParams` before
any format handler runs, so `shift` works uniformly across HLS/CMAF/TS with no
format-specific code — it simply synthesizes `startunix`, which is already
tested end-to-end.

```cpp
// Flussonic compatibility: shift=N seconds back from live == startunix=-N.
// Only applies when the caller did not send an explicit startunix/start.
if (H.GetVar("shift") != "" && !targetParams.count("startunix") && !targetParams.count("start")){
  int64_t shiftSec = atoll(H.GetVar("shift").c_str());
  if (shiftSec > 0){ targetParams["startunix"] = "-" + JSON::Value(shiftSec).asString(); }
}
```

Notes:
- `atoll` on a non-numeric string returns 0, which the `> 0` guard treats as
  "no shift" — matching the desired behavior for garbage input.
- Emitting the string `"-N"` reuses the exact same downstream path as a
  user-supplied `startunix=-N`; no new numeric/branching logic is introduced in
  `output.cpp` or `output_cmaf.cpp`.

### Alternatives considered

- **B — translate inside the unix-conversion block in `output.cpp` (~1337).**
  Rejected: runs later in the request lifecycle and duplicates the
  relative-time math that `startunix` already owns.
- **C — parse `shift` separately in each output (HLS/CMAF/TS).** Rejected:
  duplicated code across outputs, breaks the single-interception-point
  uniformity, more surface to maintain.

## Verification

This is a small deterministic transform feeding existing, tested output logic,
so verification is integration-level:

1. Build the fork locally (macOS, existing build flow).
2. Run MistServer against a live stream that has a DVR buffer of at least a few
   minutes.
3. Request the HLS manifest with `?shift=30` and confirm the first segment /
   program-date-time is ~30 s behind the live edge; confirm `?shift=0` (and no
   param) yields the plain live edge.
4. Confirm equivalence: `?shift=30` and `?startunix=-30` produce the same
   starting position.
5. Confirm precedence: `?shift=30&startunix=-120` starts ~120 s back (explicit
   `startunix` wins).
6. Sanity: `?shift=notanumber` and `?shift=-5` behave as live edge.

## Out of scope (possible future specs)

- `bandwidth=N` → variant/track selection by bitrate.
- Flussonic archive path forms (`archive-<from>-<duration>.m3u8`,
  `timeshift_abs-<utc>.m3u8`).
- Absolute-time query forms beyond what `startunix` already accepts.
