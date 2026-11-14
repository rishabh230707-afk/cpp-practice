#include "erl/hub.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
using namespace ReliefOps::Inventory;
using namespace ReliefOps::Logistics;
int checks = 0;
void check(bool ok, const char *name) {
    if (!ok)
        throw std::runtime_error(name);
    ++checks;
    std::cout << "PASS " << name << '\n';
}
template <class E, class F> void throws(F fn, const char *name) {
    bool caught = false;
    try {
        fn();
    } catch (const E &) {
        caught = true;
    }
    check(caught, name);
}
std::string bytes(const std::string &p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
int main() {
    const auto dir = std::filesystem::temp_directory_path() / "erl-hub-tests";
    // A dedicated test directory, not user data.
    std::filesystem::create_directories(dir);
    const auto path = (dir / "state.bin").string();
    std::filesystem::remove(path);
    try {
        ReliefCargo food("ration", 10, 2, 0.01);
        Manifest a(1);
        a.add(food);
        Manifest b(a);
        b.add(ReliefCargo("water", 2, 20, 0.025));
        check(a.size() == 1 && b.size() == 2, "deep copy survives independent growth");
        Manifest assigned;
        assigned = b;
        assigned = assigned;
        check(assigned == b, "copy assignment and self assignment");
        Manifest moved(std::move(assigned));
        Manifest emptyCopy(assigned);
        check(moved == b && emptyCopy.size() == 0, "move and copy moved-from manifest");
        throws<std::invalid_argument>([&] { a.add(food); }, "duplicate SKU rejected");
        throws<std::out_of_range>([&] { a.at(4); }, "manifest bounds");
        std::stringstream text;
        text << b;
        Manifest restored;
        text >> restored;
        check(restored == b, "stream manifest round trip");
        std::istringstream bad("1 \"ration\" -2 1 1");
        bad >> restored;
        check(bad.fail() && restored == b, "malformed extraction preserves destination");
        ReliefCargo precise("test space", 3, 0.123456789012345, 0.002345678901234);
        std::stringstream round;
        round << precise;
        ReliefCargo copy;
        round >> copy;
        check(copy == precise, "cargo serialization preserves floating point precision");
        DistributionDepot d;
        check(&restock(d, food) == &d, "stock return by reference");
        d += food;
        d -= food;
        check(d.available("ration") == 10, "restock and deduct operators");
        throws<std::invalid_argument>([&] { d -= ReliefCargo("ration", 11, 2, 0.01); },
                                      "insufficient inventory rejected");
        throws<std::invalid_argument>([&] { d += ReliefCargo("ration", 1, 3, 0.01); },
                                      "conflicting SKU dimensions rejected");
        Manifest volume;
        check(&allocateSupply(volume, d, "ration", 0.05) == &volume && volume.at(0).units() == 5,
              "volume allocation and reference return");
        d += ReliefCargo("medicine", 100, 0.5, 0.002);
        Manifest medical;
        allocateSupply(medical, d, "medicine", 10, 5);
        check(medical.at(0).units() == 20, "medical allocation overload");
        throws<std::invalid_argument>([&] { allocateSupply(medical, d, "medicine", 0, 5); },
                                      "invalid triage rejected");
        throws<std::invalid_argument>(
            [] { ReliefCargo c("x", 1, std::numeric_limits<double>::quiet_NaN(), 1); },
            "nonfinite weight rejected");
        int initial = TransportVehicle::operationalVehicles();
        {
            auto fleet = makeFleet();
            check(TransportVehicle::operationalVehicles() == initial + 5,
                  "static vehicle count including diamond");
            AmphibiousMedicalUnit hybrid("HYBRID-X");
            ReliefVehicle *left = static_cast<GroundConvoy *>(&hybrid);
            ReliefVehicle *right = static_cast<AmphibiousCraft *>(&hybrid);
            check(left == right, "virtual inheritance creates one shared base");
            Route r;
            r.destination = {3, 4};
            r.roadOpen = false;
            check(std::abs(left->evaluateRoute(r) - 3) < 1e-9,
                  "hybrid dynamic dispatch selects water");
            throws<DamagedCorridorException>([&] { fleet[0]->evaluateRoute(r); },
                                             "blocked ground corridor");
            r.weatherClear = false;
            throws<DamagedCorridorException>([&] { fleet[1]->evaluateRoute(r); },
                                             "unsafe aerial weather");
            r.waterOpen = false;
            throws<DamagedCorridorException>([&] { left->evaluateRoute(r); },
                                             "hybrid both corridors blocked");
            Mission m;
            m.vehicleId = "DRONE-1";
            m.cargo = a;
            throws<CargoOverweightException>([&] { fleet[2]->validateDispatch(m); },
                                             "payload overweight exception");
            m.vehicleId = "TRUCK-1";
            m.route.destination = {1000, 0};
            throws<FuelDepletionException>([&] { fleet[0]->validateDispatch(m); },
                                           "fuel depletion exception");
            m.vehicleId = "HYBRID-X";
            IncidentCommander{}.triageOverride(hybrid, m, 5);
            check(m.priority == 5 && hybrid.emergencyPriority() == 5,
                  "friend commander accesses private triage state");
            m.vehicleId = "DRONE-1";
            throws<std::invalid_argument>(
                [&] { IncidentCommander{}.triageOverride(*fleet[2], m, 5); },
                "nonmedical drone override rejected");
        }
        check(TransportVehicle::operationalVehicles() == initial,
              "virtual destructors release entire fleet");
        PriorityDispatchQueue<Mission, UrgentMedicalFirst> q;
        Mission lo, hi, tie;
        lo.id = 3;
        hi.id = 2;
        hi.priority = 5;
        tie.id = 1;
        tie.priority = 5;
        q.push(lo);
        q.push(hi);
        q.push(tie);
        check(q.top().id == 1, "priority queue tie by mission ID");
        q.pop();
        check(q.top().id == 2, "urgent medical mission before routine");
        q.pop();
        q.pop();
        throws<std::out_of_range>([&] { q.top(); }, "empty queue guarded");
        PriorityDispatchQueue<int> ints;
        ints.push(3);
        ints.push(1);
        check(ints.top() == 1, "generic queue supports another type");
        {
            ReliefHub h(path);
            h.restock(ReliefCargo("ration", 100, 2, 0.01));
            Mission m;
            m.vehicleId = "TRUCK-1";
            m.route.destination = {3, 4};
            m.cargo = a;
            auto before = bytes(path);
            double fuel = h.vehicle("TRUCK-1").fuel();
            throws<std::runtime_error>([&] { h.dispatch(m, "NDRF", true); },
                                       "injected failure before commit");
            check(bytes(path) == before && h.records().empty() &&
                      h.depot().available("ration") == 100 && h.vehicle("TRUCK-1").fuel() == fuel,
                  "failed transaction preserves disk inventory fuel and audit");
            throws<std::runtime_error>([&] { h.restock(food, true); }, "restock rollback");
            check(h.depot().available("ration") == 100, "restock rollback preserves memory");
            h.dispatch(m, "NDRF");
            check(h.depot().available("ration") == 90 && h.records().size() == 1,
                  "successful dispatch deducts stock and logs manifest");
            check(std::abs(h.vehicle("TRUCK-1").fuel() - 298.25) < 1e-9 &&
                      std::abs(ReliefHub::dispatchedTonnes() - 0.02) < 1e-9,
                  "fuel and tonnage analytics");
            throws<std::invalid_argument>([&] { h.dispatch(m, "NDRF"); },
                                          "duplicate mission ID prevented");
            m.id = 2;
            m.cargo.add(ReliefCargo("medicine", 1, 0.5, 0.002));
            before = bytes(path);
            throws<std::invalid_argument>([&] { h.dispatch(m, "Medical team"); },
                                          "multi-item stock shortage");
            check(h.depot().available("ration") == 90 && bytes(path) == before,
                  "no partial multi-item deduction");
        }
        {
            ReliefHub h(path);
            check(h.records().size() == 1 && h.records()[0].mission.cargo == a &&
                      h.depot().available("ration") == 90,
                  "restart restores stock logs and manifests");
            check(std::abs(h.vehicle("TRUCK-1").fuel() - 298.25) < 1e-9 &&
                      h.vehicle("TRUCK-1").position().x == 3,
                  "restart replays vehicle fuel and position");
        }
        {
            std::fstream f(path, std::ios::binary | std::ios::in | std::ios::out);
            f.seekp(20);
            f.put('X');
        }
        throws<std::runtime_error>([&] { BinaryStore(path).load(); },
                                   "checksum corruption detected");
        {
            std::ofstream f(path, std::ios::binary | std::ios::trunc);
            f << "ERLHUB01";
        }
        throws<std::runtime_error>([&] { BinaryStore(path).load(); },
                                   "truncated binary header detected");
        throws<std::runtime_error>(
            [&] { BinaryStore((dir / "missing" / "state.bin").string()).save({}); },
            "unwritable destination reported");
        std::filesystem::remove(path);
        std::filesystem::remove(dir);
        std::cout << "All " << checks << " checks passed.\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
