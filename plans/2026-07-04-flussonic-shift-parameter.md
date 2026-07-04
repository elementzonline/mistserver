# Flussonic `shift` Query Parameter — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Accept Flussonic-style `?shift=N` on playback URLs, meaning "start N seconds behind the live edge," by translating it into MistServer's existing `startunix=-N`.

**Architecture:** A pure helper in libmist (`HTTP::flussonicShiftToStartunix`) converts a `shift` string into a `startunix` string (or empty). `HTTPOutput::onHTTP()` calls it at the single query-capture point, so the synthesized `startunix` flows through the already-tested relative-time logic and reaches every HTTP output (HLS, CMAF, TS) unchanged.

**Tech Stack:** C++, Meson build, MistServer's existing `test/` unit-test harness (small binaries linked against `libmist_dep`, driven by env vars).

## Global Constraints

- Scope is `shift` only. Do NOT implement `bandwidth`, archive path forms, or absolute-time query forms.
- `shift=N` (N > 0 integer, seconds) → synthesize `startunix = -N`.
- `shift` absent, `0`, negative, or non-numeric → no shift (normal live edge).
- If the request already carries an explicit `startunix` or `start`, it wins; `shift` is ignored (do not override).
- Reuse existing downstream logic — introduce NO new numeric/seek math in `output.cpp` or `output_cmaf.cpp`.
- Follow existing code idioms: `atoll` for parsing, `JSON::Value(x).asString()` for int→string, `<mist/...>` include path in tests, `test(...)` registration in `test/meson.build`.

---

## File Structure

- **Modify** `lib/http_parser.h` — declare `std::string HTTP::flussonicShiftToStartunix(const std::string &shiftVal);` next to `parseVars` (inside `namespace HTTP{`, before the closing `}` at line 87).
- **Modify** `lib/http_parser.cpp` — define the helper. `json.h` is already included (line 11); add `#include <cstdlib>` for `atoll` if not already transitively available.
- **Create** `test/flussonic_shift.cpp` — unit-test driver for the helper.
- **Modify** `test/meson.build` — register the test binary and its cases.
- **Modify** `src/output/output_http.cpp` — call the helper at the query-capture point (~after line 433), guarded so explicit `startunix`/`start` wins.

---

### Task 1: `HTTP::flussonicShiftToStartunix` helper + unit tests

**Files:**
- Modify: `lib/http_parser.h` (add declaration inside `namespace HTTP{`, near line 17)
- Modify: `lib/http_parser.cpp` (add definition; add `#include <cstdlib>` near line 12 if needed)
- Create: `test/flussonic_shift.cpp`
- Modify: `test/meson.build` (register binary + tests, near the other `executable(...)`/`test(...)` blocks)

**Interfaces:**
- Produces: `std::string HTTP::flussonicShiftToStartunix(const std::string &shiftVal)` — returns `"-N"` (string) when `shiftVal` parses to a positive integer N; returns `""` (empty string) otherwise. Consumed by Task 2.

- [ ] **Step 1: Declare the helper with an empty stub (compiles, but wrong)**

In `lib/http_parser.h`, inside `namespace HTTP{`, right after the `parseVars(...)` declaration (line 17), add:

```cpp
  /// Flussonic compatibility: converts a "shift" query value (seconds behind
  /// live) into the equivalent "startunix" value ("-N"). Returns an empty
  /// string when shiftVal is absent, zero, negative, or non-numeric.
  std::string flussonicShiftToStartunix(const std::string &shiftVal);
```

In `lib/http_parser.cpp`, add the stub near the top-level function definitions (e.g. just after the `parseVars` definition). Stub returns empty so the code compiles but the positive-shift tests will fail:

```cpp
std::string HTTP::flussonicShiftToStartunix(const std::string &shiftVal){
  return "";
}
```

- [ ] **Step 2: Write the failing tests**

Create `test/flussonic_shift.cpp`:

```cpp
#include <cstdlib>
#include <iostream>
#include <string>
#include <mist/http_parser.h>

int main(){
  const char *inRaw = getenv("T_SHIFT");
  const char *expRaw = getenv("T_STARTUNIX");
  std::string input = inRaw ? inRaw : "";
  std::string expected = expRaw ? expRaw : "";
  std::string got = HTTP::flussonicShiftToStartunix(input);
  if (got != expected){
    std::cerr << "flussonicShiftToStartunix(\"" << input << "\") = \"" << got
              << "\", expected \"" << expected << "\"" << std::endl;
    return 1;
  }
  return 0;
}
```

Register it in `test/meson.build` (append after the last `test(...)` block, before the trailing commented-out `abst_test`):

```meson
flussonicshifttest = executable('flussonicshifttest', 'flussonic_shift.cpp', header_tgts, dependencies: libmist_dep)
test('Positive shift maps to negative startunix', flussonicshifttest, suite: 'Flussonic shift', env: {'T_SHIFT':'100', 'T_STARTUNIX':'-100'})
test('Example bandwidth-style value', flussonicshifttest, suite: 'Flussonic shift', env: {'T_SHIFT':'2200', 'T_STARTUNIX':'-2200'})
test('Zero shift is live edge', flussonicshifttest, suite: 'Flussonic shift', env: {'T_SHIFT':'0', 'T_STARTUNIX':''})
test('Negative shift is live edge', flussonicshifttest, suite: 'Flussonic shift', env: {'T_SHIFT':'-5', 'T_STARTUNIX':''})
test('Non-numeric shift is live edge', flussonicshifttest, suite: 'Flussonic shift', env: {'T_SHIFT':'abc', 'T_STARTUNIX':''})
test('Empty shift is live edge', flussonicshifttest, suite: 'Flussonic shift', env: {'T_SHIFT':'', 'T_STARTUNIX':''})
```

- [ ] **Step 3: Build and run — verify the positive-shift tests FAIL**

Run:
```bash
meson compile -C build && meson test -C build --suite 'Flussonic shift' --print-errorlogs
```
Expected: the "Positive shift..." and "Example bandwidth-style value" cases FAIL (stub returns `""` but `-100`/`-2200` expected). The four live-edge cases PASS.

> If `build/` is not yet configured, run `meson setup build` once first.

- [ ] **Step 4: Implement the real helper**

Replace the stub body in `lib/http_parser.cpp`:

```cpp
std::string HTTP::flussonicShiftToStartunix(const std::string &shiftVal){
  int64_t shiftSec = atoll(shiftVal.c_str());
  if (shiftSec <= 0){return "";}
  return "-" + JSON::Value(shiftSec).asString();
}
```

(`JSON::Value` is available — `json.h` is included at line 11. `atoll` needs `<cstdlib>`; add `#include <cstdlib>` near line 12 if compilation complains about `atoll`.)

- [ ] **Step 5: Build and run — verify ALL Flussonic cases PASS**

Run:
```bash
meson compile -C build && meson test -C build --suite 'Flussonic shift' --print-errorlogs
```
Expected: all six cases PASS.

- [ ] **Step 6: Commit**

```bash
git add lib/http_parser.h lib/http_parser.cpp test/flussonic_shift.cpp test/meson.build
git commit -m "Add HTTP::flussonicShiftToStartunix helper with unit tests"
```

---

### Task 2: Wire `shift` into the HTTP query-capture point

**Files:**
- Modify: `src/output/output_http.cpp` (after the `HTTP_CONVERT(...)` block, ~line 433)

**Interfaces:**
- Consumes: `HTTP::flussonicShiftToStartunix(const std::string &)` from Task 1.
- Produces: sets `targetParams["startunix"]` when a positive `shift` is present and neither `startunix` nor `start` was supplied. No new public symbols.

- [ ] **Step 1: Add the call site**

In `src/output/output_http.cpp`, immediately after the final `HTTP_CONVERT("maxwaittrackms");` line (line 433) and before the `buffer` handling comment (line 435), insert:

```cpp
      // Flussonic compatibility: shift=N means N seconds behind live == startunix=-N.
      // An explicit startunix/start always takes precedence over shift.
      if (H.GetVar("shift") != "" && !targetParams.count("startunix") && !targetParams.count("start")){
        std::string shiftUnix = HTTP::flussonicShiftToStartunix(H.GetVar("shift"));
        if (shiftUnix.size()){targetParams["startunix"] = shiftUnix;}
      }
```

(`output_http.cpp` already uses `H.GetVar(...)`, `targetParams`, and the `HTTP` namespace, and includes the HTTP parser header — no new includes needed. Confirm `HTTP::flussonicShiftToStartunix` resolves; if the file only sees the parser via a forward include, add `#include <mist/http_parser.h>` — but it is already available since `HTTP_CONVERT` uses `H.GetVar`.)

- [ ] **Step 2: Build — verify it compiles**

Run:
```bash
meson compile -C build
```
Expected: builds with no errors.

- [ ] **Step 3: Regression — full unit test suite still passes**

Run:
```bash
meson test -C build --print-errorlogs
```
Expected: all tests PASS (including the six `Flussonic shift` cases from Task 1).

- [ ] **Step 4: Commit**

```bash
git add src/output/output_http.cpp
git commit -m "Translate Flussonic shift query param into startunix"
```

---

### Task 3: Integration verification (manual)

**Files:** none (verification only — no code changes; do not commit).

This confirms the end-to-end behavior that unit tests cannot: the synthesized `startunix` actually shifts the playback window. Requires a running MistServer built from this branch and a live stream with a DVR buffer of at least a few minutes.

- [ ] **Step 1: Start the built MistServer and a live source**

Use the fork's normal local run flow (macOS). Ensure a live stream (call it `STREAM`) is ingesting and has a recording/DVR window of ≥ 5 minutes.

- [ ] **Step 2: Confirm equivalence of shift and startunix**

Run:
```bash
curl -s 'http://localhost:8080/STREAM/index.m3u8?shift=30'    > /tmp/shift30.m3u8
curl -s 'http://localhost:8080/STREAM/index.m3u8?startunix=-30' > /tmp/startunix30.m3u8
diff /tmp/shift30.m3u8 /tmp/startunix30.m3u8 && echo "EQUIVALENT"
```
Expected: manifests reference the same starting media position (`EXT-X-PROGRAM-DATE-TIME` / first segment ~30 s behind the live edge). Adjust host/port to your deployment.

- [ ] **Step 3: Confirm live edge and precedence**

Run:
```bash
curl -s 'http://localhost:8080/STREAM/index.m3u8?shift=0'                 # expect live edge
curl -s 'http://localhost:8080/STREAM/index.m3u8'                        # expect live edge (identical)
curl -s 'http://localhost:8080/STREAM/index.m3u8?shift=30&startunix=-120' # expect ~120s back (startunix wins)
curl -s 'http://localhost:8080/STREAM/index.m3u8?shift=abc'              # expect live edge
```
Expected: `shift=0`, no-param, and `shift=abc` all yield the live edge; the `shift=30&startunix=-120` case starts ~120 s back, proving precedence.

- [ ] **Step 4: (Optional) Confirm the real client URL form works**

Run:
```bash
curl -s 'http://localhost:8080/STREAM/index.m3u8?bandwidth=2200&shift=0'
```
Expected: a valid live manifest — `bandwidth` is ignored (out of scope) and `shift=0` yields the live edge.

---

## Self-Review

- **Spec coverage:** shift>0→startunix=-N (Task 1 + Task 2, Task 3 Step 2); shift=0/absent/negative/non-numeric→live (Task 1 tests + Task 3 Step 3); explicit startunix/start wins (Task 2 guard + Task 3 Step 3); shift beyond buffer clamps (existing logic, unchanged — no task needed); shift+duration side effect (existing `duration` flow, unchanged — no new code); uniform across HLS/CMAF/TS (single interception point, Task 2). All spec points covered.
- **Placeholder scan:** none — every code and command step is concrete.
- **Type consistency:** `flussonicShiftToStartunix(const std::string&) -> std::string` used identically in the declaration (Task 1 Step 1), definition (Task 1 Step 4), test (Task 1 Step 2), and call site (Task 2 Step 1).
