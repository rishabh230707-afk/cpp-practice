# ERL-HUB

**Post-Earthquake Multi-Agency Emergency Relief Logistics Hub**  
CIC-211 Object-Oriented Programming project · C++17 · MAIT, Delhi  
Developed by Rishabh Bansal · Individual submission

ERL-HUB coordinates relief stock, heterogeneous vehicles and priority missions after a simulated earthquake. It demonstrates all four units of the supplied CIC-211 project specification through executable behavior and tests.

## Build and run

No third-party C++ libraries are needed. Use a C++17 compiler (GCC 9+ or a recent Apple Clang) and Make.

```sh
make
./build/erl-hub
```

On a Mac, install Apple's command-line developer tools if they are missing (`xcode-select --install`), then run the commands above from the project directory. The direct compiler equivalent is:

```sh
c++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/inventory.cpp src/logistics.cpp src/hub.cpp src/main.cpp -o erl-hub
./erl-hub
```

Run the reproducible demonstration with a **new data filename**:

```sh
./build/erl-hub --demo --data demo.bin
```

This adds sample relief stock, queues a routine ration mission and a priority-5 medical mission, dispatches medical supplies first through the hybrid vehicle's water route, and saves both manifests. Re-running the demo adds another batch of stock and missions; it does not reset your data.

## Interactive workflow

1. Choose **7** once to seed ration, medicine, water and blanket stock, or **3** to enter cargo.
2. Choose **1/2** to inspect inventory and the fleet.
3. Choose **4** to queue a mission. Supply a unique positive ID, vehicle ID, priority (5 highest), local destination coordinates, corridor status and allocation method.
4. Choose **5** to dispatch the highest-priority mission, naming its agency (for example NDRF, paramilitary relief convoy or emergency medical team).
5. Choose **6/8** to inspect audit manifests and statistics.
6. Exit with **0**. Restart using the same `--data` filename to restore stock, committed manifests, fuel and vehicle positions.

Vehicles: `TRUCK-1`, `HELI-1`, `DRONE-1`, `BOAT-1`, `HYBRID-1`.

Example restock input: `medicine 100 0.5 0.002` (SKU, units, kilograms per unit, cubic metres per unit). Reusing an SKU with different dimensions is rejected. Queued missions reserve no stock; availability is rechecked on departure. Failed missions remain queued until successfully dispatched or explicitly removed.

## Verification

```sh
make test
make sanitize
```

The regression suite covers deep copies, stream parsing, polymorphism, diamond identity, virtual destruction, both allocation overloads, payload/fuel/route failures, queue ordering, stock rollback, binary corruption and restart recovery. On traced containers where LeakSanitizer cannot run, use `make sanitize ASAN_OPTIONS=detect_leaks=0`; that still runs AddressSanitizer and UndefinedBehaviorSanitizer, but does **not** check leaks. See [test evidence](docs/TEST_REPORT.md).

## Documentation

- [Project report and design](docs/PROJECT_REPORT.md)
- [Every specification requirement mapped to code](docs/REQUIREMENTS.md)
- [Viva questions and demonstration script](docs/VIVA.md)
- [Test results and limitations](docs/TEST_REPORT.md)
- [Actual demonstration output](examples/demo-output.txt)

## Scope and persistence guarantees

This is a single-process educational simulation, using local Cartesian kilometres and synthetic fuel costs; it does not connect to real emergency services. The binary file contains a version header, little-endian length and checksum followed by a length-delimited, precisely serialized payload. It never dumps object pointers or compiler-dependent class layouts.

Each dispatch copies state, validates all stock and vehicle constraints, writes and verifies a sibling temporary file, and atomically replaces the old file with `std::rename` on macOS/Linux. Inventory, log and manifest commit together. The original is retained on failure before replacement. Standard C++ does not provide `fsync`, so this is atomic visibility and application-failure protection, **not** a guarantee against power-loss durability. On platforms where rename cannot replace an existing file, the operation reports an error and retains the original; it never deletes the original first.

Use one running hub per data file. Pending queues and commander preference are session-only; committed dispatches are persistent. Vehicles represent available resources and routes complete immediately in this simulation. Do not use the library-level `refuel` method on a persistent hub: external vehicle modifications are not logged; the console does not expose them. Statistics describe the active hub's restored dispatch history; run one hub per process.

## Submission

The source archive includes all code, tests, documentation and example output; build products and live data are excluded. Deadline provided in the assignment: **27 November 2026, 11:59 PM**. Submit through the supplied [Google Form](https://forms.gle/cgWY4vZ3Siep8mmF9), and provide the final GitHub repository as required. 
