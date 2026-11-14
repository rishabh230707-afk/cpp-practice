# CIC-211 requirement traceability

This matrix follows Option 2 of the supplied assignment, not Option 1.

| Unit | Required mechanism | Implementation and observable evidence |
|---|---|---|
| I | `ReliefOps::Logistics` / `ReliefOps::Inventory` namespaces | All public declarations and definitions live in the respective nested namespaces; `include/erl/*.hpp`, `src/*.cpp`. |
| I | Allocation function overloading | `allocateSupply(Manifest&, const DistributionDepot&, const string&, double)` handles a volume budget; the overload ending in `int patients, int priority` handles triage medicine. Both are in `inventory.cpp`, exercised by demo and tests. |
| I | Call by reference | `restock`, `allocateSupply`, `updateMissionPriority`, and commander override mutate caller-owned objects. Read-only arguments use const references. |
| I | Return by reference | `restock` returns the same depot; allocation returns the same manifest; priority updates return the same mission. Reference identity is tested. |
| I | Inline distance/payload utilities | `distanceKm` and `tonnes` in `logistics.hpp`; class-defined accessors are implicitly inline. |
| I | Friend `IncidentCommander` | Declared friend by `ReliefVehicle`; `triageOverride` directly writes private `emergencyPriority_`, updating mission priority while preserving safety constraints. |
| II | ReliefCargo, DistributionDepot, TransportVehicle ADTs | Private representation, validated construction, read-only queries, invariant-preserving restock/deduct/refuel operations. |
| II | Static analytics | `TransportVehicle::operational_` and `operationalVehicles()`; `ReliefHub::dispatchedTonnes_` and `dispatchedTonnes()`. The active hub restores tonnage from its log. |
| II | Parameterized/dynamic constructors | `Manifest(size_t)` dynamically allocates `ReliefCargo[]`; cargo and vehicle constructors receive validated configuration. |
| II | Deep copy constructor | Manifest copies its owned heap array, with independent resizing verified by regression tests. |
| II | Explicit destructors | Manifest uses `delete[]`; transport has a virtual destructor and decrements the live count. Rule-of-Five assignment/move support prevents double deletion. |
| II | `+=` restock | `DistributionDepot::operator+=` validates matching dimensions and integer overflow. Used by restock and state restoration. |
| II | `-=` on departure | `ReliefHub::dispatch` applies `operator-=` to each manifest item in staged state before commit. |
| II | `==` identity / duplicates | Cargo equality compares SKU, quantity and dimensions. Manifest equality compares content independently of order. Repeated SKU entries and mission IDs are rejected; equality round trips are tested. |
| II | `<<`, `>>` and serialization | Both Cargo and Manifest support insertion/extraction. BinaryStore uses those operators to serialize and restore manifest payloads, and CLI restock uses extraction. |
| III | ReliefVehicle hierarchy | Abstract `ReliefVehicle` derives from TransportVehicle. GroundConvoy, AerialTransport and AmphibiousCraft provide virtual route evaluations. AerialTransport represents medical helicopter or rescue drone. |
| III | Diamond / virtual base | GroundConvoy and AmphibiousCraft virtually inherit ReliefVehicle; AmphibiousMedicalUnit inherits both and explicitly constructs the shared virtual base. Test compares both base addresses. |
| III | GPS and life-support containership | ReliefVehicle owns `GPSBeacon` and `EmergencyLifeSupportUnit` values, independent types declared in `logistics.hpp`. These are has-a relationships. |
| III | Base-pointer runtime dispatch | `ReliefHub::fleet()` returns `vector<ReliefVehicle*>` views. CLI invokes virtual `type()` through them; hub and tests invoke virtual `evaluateRoute()` through base references/pointers. Owning `unique_ptr<ReliefVehicle>` ensures virtual destruction. |
| IV | std::exception framework | LogisticsException owns a diagnostic string and overrides noexcept `what()`; CargoOverweightException, FuelDepletionException and DamagedCorridorException derive from it. Each failure is tested. |
| IV | Binary fstream persistence | BinaryStore reads/writes a versioned binary envelope, stock and full audit manifests; restart replay restores vehicle fuel/positions. |
| IV | Atomic updates | Write sibling temporary file, flush, close, read-back validation, then rename. State swaps and vehicle updates after commit are nonthrowing. Injected failure confirms unchanged file, inventory, fuel and log. Platform guarantees are documented in README. |
| IV | Generic `PriorityDispatchQueue<T>` | Template in `logistics.hpp` with optional comparator. Tested with Mission and int. |
| IV | vector and unordered_map | Vectors hold fleet, queue and audit records; unordered_map indexes depot stock by SKU. |
| IV | Custom sorting functor | UrgentMedicalFirst orders higher urgency first, breaking ties with lower mission ID; used by stable_sort inside the queue. |
| IV | Standard algorithms | sort produces deterministic inventory order; stable_sort orders missions; for_each displays base-pointer fleet; find detects duplicate saved IDs. |
| IV | User-defined I/O manipulator | `manifestHeader(ostream&)`, used as `out << manifestHeader`; `printManifest` uses iomanip widths/precision and restores formatting flags. |
| General | Modular standard C++ | Three domain headers, three implementation modules and one CLI; C++17, standard library only; strict warning build. |
| General | Team size at most two | Individual project by Rishabh Bansal. |

## Requirement terminology

“Dynamic constructor” means a constructor allocating dynamic memory, not a separate C++ language feature. Composition here means owned objects inside a vehicle. In strict UML terminology this is composition, whereas non-owning `ReliefVehicle*` fleet views are aggregation. Virtual calls demonstrate runtime polymorphism; vtables/vptrs are common compiler implementation mechanisms, not named facilities guaranteed by the C++ standard.
