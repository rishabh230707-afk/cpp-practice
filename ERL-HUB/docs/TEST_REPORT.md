# Verification report

**Date:** 14 November 2026  
**Environment:** Linux; GCC 14.2.0; C++17  
**Compiler flags:** `-std=c++17 -Wall -Wextra -Wpedantic -Werror -O2`

| Check | Observed result |
|---|---|
| Strict application build | Passed with warnings treated as errors |
| Automated functional regression suite | 45 checks passed |
| AddressSanitizer + UndefinedBehaviorSanitizer, leak detection disabled | 45 checks passed; no sanitizer diagnostics |
| LeakSanitizer | Could not run: environment reports that LeakSanitizer does not work under ptrace. A leak-free result is not claimed. |
| End-to-end demo | Medical mission dispatched first; hybrid used water with road blocked; routine mission followed; 0.11 tonnes logged |
| Restart smoke test | Persisted stock, two audit manifests, reduced vehicle fuel and dispatch statistics displayed successfully |
| macOS execution | Instructions provided; not executed on a Mac in this environment |

## Reproduce

From the project directory:

```sh
make all test
make sanitize
./build/erl-hub --demo --data demo-new.bin
./build/erl-hub --data demo-new.bin
```

For a traced environment only: `make sanitize ASAN_OPTIONS=detect_leaks=0`.

## Coverage

- Dynamic arrays: independent deep copies, resizing, copy assignment, self-assignment, move construction and moved-from copying.
- Serialization: precise floating-point cargo round trip, complete manifest identity, invalid extraction preserving the destination.
- Inventory: both compound operators, reference return identity, insufficient stock, dimension conflicts, invalid triage, nonfinite cargo values.
- Inheritance: one shared virtual base, hybrid route dispatch, blocked road/water/weather, static live counts and base-pointer destruction.
- Dispatch: weight failure, fuel failure, commander private access, nonmedical override rejection, priority ordering and tie breaking.
- Templates: queue instantiated for Mission and int, empty-queue protection.
- Persistence: save failure injection, unchanged file/inventory/fuel/log on failure, restock rollback, duplicate mission detection, multi-item shortage without partial deduction, restart replay, corruption/truncation detection and invalid destination directory.

The test logs in `examples/` are captured program output, not hand-written sample results. The test executable returns a nonzero status on a failed assertion; it does not depend on the C assert macro or NDEBUG.

## Remaining scope limits

This testing does not certify a real disaster-management system. The application is single-process, uses a fixed fleet and synthetic routes, and stores only committed missions. Atomic rename does not establish power-loss durability. These boundaries are also documented in the README and project report.
