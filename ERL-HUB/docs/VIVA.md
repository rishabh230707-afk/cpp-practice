# Viva guide and demonstration

## Five-minute demonstration

1. Build using `make` and run `make test`.
2. Run `./build/erl-hub --demo --data viva-new.bin` with a fresh filename. Explain that priority 5 goes first even though the ration mission was queued first.
3. Explain that HYBRID-1 uses water because the road is blocked. Point to its evaluateRoute override.
4. Restart `./build/erl-hub --data viva-new.bin`, choose 1, 2, 6 and 8 to show restored stock, fleet fuel, manifest audits and analytics.
5. Show tests for the virtual-base address identity and destructor counter, then the injected save failure test that preserves disk, stock, log and fuel.
6. Open REQUIREMENTS.md and navigate to one implementation per unit.

## Questions and concise answers

**Why two namespaces?** Inventory contains stock and cargo abstractions; Logistics contains vehicles, missions and dispatch. Namespaces organize names and prevent clashes.

**Where is function overloading?** The two allocateSupply functions accept different final parameter types and counts: a double volume budget, or int patients and int priority. The compiler selects one at compile time.

**Where is return by reference?** restock returns DistributionDepot& and allocateSupply returns Manifest&. These references point to objects owned by the caller, so no temporary or local object is returned.

**What does inline guarantee?** It permits identical function definitions across translation units under the one-definition rules. It does not force machine-code inlining or promise better speed.

**Why a friend class?** IncidentCommander demonstrates controlled access to ReliefVehicle's private triage field. Only that class receives access; ordinary callers use public methods.

**What is an ADT?** A type defined by its operations and behavior while hiding its representation. DistributionDepot exposes restocking and deduction without exposing its unordered_map.

**Why a deep copy?** A shallow pointer copy would make two manifests own the same array, leading to shared mutations and double deletion. The copy constructor allocates a separate array and copies the cargo values.

**What is the Rule of Five?** A resource-owning class may need a destructor, copy constructor, copy assignment, move constructor and move assignment. Here assignment takes by value and swaps, supporting copy and move assignment through a single implementation.

**What is a dynamic constructor?** Informal syllabus terminology for a constructor that allocates memory dynamically. Manifest(size_t) uses new ReliefCargo[capacity].

**Why are operators member/friend functions?** += and -= mutate their left-hand depot and return its reference. Stream operators are nonmembers because the left operand is an istream or ostream; friendship grants access to private fields.

**What is the diamond problem?** The hybrid inherits GroundConvoy and AmphibiousCraft, both based on ReliefVehicle. Without virtual inheritance it would have two ReliefVehicle subobjects. Virtual inheritance shares one, initialized by the most-derived hybrid constructor.

**How is composition different from inheritance?** A hybrid is a vehicle, but a vehicle has a GPS beacon and life-support unit. Owned member objects express the latter relationship.

**Where is runtime polymorphism?** Calling evaluateRoute through ReliefVehicle& or ReliefVehicle* invokes the concrete truck, aerial, boat or hybrid implementation. A virtual destructor permits safe deletion through a base pointer.

**Does C++ require a vtable?** No. Vtables and vptrs are common implementation techniques. The standard requires observable virtual-dispatch semantics, not a particular memory layout.

**Why a template queue?** PriorityDispatchQueue works with multiple types. Mission uses UrgentMedicalFirst while int uses std::less. Both are tested.

**Why a functor instead of a plain function?** A functor is an object with operator(). It satisfies algorithm comparator requirements and can hold policy state if later required. This one orders priority and mission ID.

**What happens on a failed dispatch?** The original state remains unchanged because validation and deductions use a copy. The file commits before nonthrowing in-memory swaps. Failed queued missions remain available for retry or removal.

**Why not write sizeof(Manifest) bytes to disk?** That would serialize a pointer and implementation-dependent layout, not the array contents. Each value is serialized explicitly within a versioned binary envelope.

**Is the save fully crash-proof?** Atomic rename prevents readers seeing a partially replaced file on supported filesystems. Standard C++ does not guarantee durable storage after power loss, so the project does not claim that.

**Are pending missions saved?** No. Stock and completed dispatch audit manifests are saved; the pending queue is session-only. Explain this distinction clearly.

**Why not add AI or live maps?** They are not needed for the stated OOP requirements. This project keeps its core behavior deterministic and explainable.
