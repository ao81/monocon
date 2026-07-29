# Monocon Tools RASIS release gates

This document defines the evidence required before an engine build is treated
as a contest-ready release. A green build alone is not sufficient.

## Reliability

- Compile behavior must match Arduino AVR Boards 1.8.7 for Mega 2560:
  GNU++11, `-fpermissive`, LTO, Arduino `.ino` ordering, generated
  prototypes, local `src/` sources, and supported bundled libraries.
- Every cache hit must verify source dependencies, object contents, ELF,
  HEX, build flags, compiler identity, and `core.a`.
- Intel HEX parsing must reject malformed lengths, checksums, overlapping
  addresses, out-of-range data, and data in the 8 KiB bootloader region.
- Upload success requires the ATmega2560 signature and byte-for-byte
  readback of every programmed page.

Evidence:

- `npm test` in the extension directory passes all tests.
- `test/native-builder.test.js` passes with a fresh temporary cache.
- `test/HARDWARE_VALIDATION.md` contains a successful current-engine
  hardware result.

## Availability

- The extension-native worker is the primary path.
- A statically linked CLI and daemon provide an offline fallback when the
  native addon cannot start.
- Native startup, compilation, upload, serial operations, and VS Code tasks
  have bounded waits.
- A timed-out synchronous native upload remains isolated until its real
  completion, preventing a second write or premature monitor reopen.
- Transient worker-construction and worker-crash paths must be retryable.

Evidence:

- Native worker fault-injection tests pass.
- Standalone daemon integration tests pass without system Arduino tools.
- PE dependency inspection shows only Windows system DLLs.

## Serviceability

- Compiler output is normalized, translated into concise Japanese guidance,
  and published to the VS Code Problems panel.
- Output records compile mode, timing, selected port, upload phase timings,
  page counts, byte counts, and readback counts.
- Cache corruption is self-repairing; old caches and stale intermediate
  files are bounded and pruned under cross-process locks.
- The compatibility daemon logs to
  `%LOCALAPPDATA%\ArduinoBuildDaemon\daemon.log`.

## Integrity

- The extension never silently loads an older native engine.
- Engine-generation pipe and mutex names prevent cross-version IPC.
- Build-cache and COM-port mutexes prevent cross-process races.
- Child compiler processes inherit only stdin, stdout, and stderr handles.
- VSIX inspection must show exactly one expected `.node`, no old engines,
  no test/build intermediates, and an archive hash matching the disk binary.

## Security

- Daemon IPC uses an owner-only DACL, rejects remote clients, limits requests
  to 1 MiB, and fails closed if the security descriptor cannot be created.
- CLI completion notifications use an unpredictable per-execution pipe.
- Runtime settings, paths, COM names, source sizes, process output, metadata,
  and HEX sizes are bounded.
- Release binaries require high-entropy ASLR, DEP/NX, and Control Flow Guard.

## Current residual release decisions

- Authenticode signing requires a project-owned code-signing certificate.
- Public distribution requires the project owner to choose a top-level
  license; third-party source notices and bundled license files must remain.
- Hardware qualification currently proves one Arduino Mega ADK. Official
  Mega 2560 and common CH340/FTDI/CP210x clone matrices should be repeated
  when those boards are available.
