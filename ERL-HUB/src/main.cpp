#include "erl/hub.hpp"
#include <iostream>
#include <sstream>
using namespace ReliefOps::Inventory;
using namespace ReliefOps::Logistics;
namespace {
std::string line(const std::string &prompt) {
    std::cout << prompt;
    std::string s;
    if (!std::getline(std::cin, s))
        throw std::runtime_error("Input ended");
    return s;
}
template <class T> T number(const std::string &prompt) {
    std::istringstream in(line(prompt));
    T n;
    if (!(in >> n))
        throw std::invalid_argument("Expected a number");
    in >> std::ws;
    if (!in.eof())
        throw std::invalid_argument("Unexpected input after number");
    return n;
}
bool flag(const std::string &p) {
    int n = number<int>(p);
    if (n != 0 && n != 1)
        throw std::invalid_argument("Enter 0 or 1");
    return n == 1;
}
void seed(ReliefHub &h) {
    h.restock(ReliefCargo("ration", 1000, 2, 0.01));
    h.restock(ReliefCargo("medicine", 500, 0.5, 0.002));
    h.restock(ReliefCargo("water", 200, 20, 0.025));
    h.restock(ReliefCargo("blanket", 500, 1, 0.008));
}
void inventory(const ReliefHub &h) {
    std::cout << manifestHeader;
    for (const auto &c : h.depot().items())
        std::cout << c << '\n';
}
void fleet(const ReliefHub &h) {
    auto views = h.fleet();
    std::for_each(views.begin(), views.end(), [](ReliefVehicle *v) {
        std::cout << v->id() << " | " << v->type() << " | fuel " << v->fuel() << " | payload "
                  << v->maxKg() << " kg\n";
    });
}
void demo(ReliefHub &h) {
    seed(h);
    Mission medical;
    medical.id = static_cast<int>(h.records().size()) + 1;
    medical.vehicleId = "HYBRID-1";
    medical.route.destination = {10, 10};
    medical.route.roadOpen = false;
    allocateSupply(medical.cargo, h.depot(), "medicine", 10, 5);
    IncidentCommander{}.triageOverride(h.vehicle(medical.vehicleId), medical, 5);
    Mission food;
    food.id = medical.id + 1;
    food.vehicleId = "TRUCK-1";
    food.route.destination = {5, 0};
    allocateSupply(food.cargo, h.depot(), "ration", 0.5);
    PriorityDispatchQueue<Mission, UrgentMedicalFirst> queue;
    queue.push(food);
    queue.push(medical);
    while (!queue.empty()) {
        auto m = queue.top();
        h.dispatch(m, m.priority == 5 ? "Emergency medical team" : "NDRF");
        std::cout << "Dispatched mission " << m.id << " priority " << m.priority << " using "
                  << m.vehicleId << '\n';
        printManifest(std::cout, m.cargo);
        queue.pop();
    }
    fleet(h);
    std::cout << "Dispatched metric tonnes: " << ReliefHub::dispatchedTonnes() << '\n';
}
} // namespace
int main(int argc, char **argv) {
    bool demoMode = false;
    std::string path = "erl_state.bin";
    for (int j = 1; j < argc; ++j) {
        std::string a = argv[j];
        if (a == "--demo")
            demoMode = true;
        else if (a == "--data" && j + 1 < argc)
            path = argv[++j];
        else {
            std::cerr << "Usage: erl-hub [--demo] [--data FILE]\n";
            return 2;
        }
    }
    try {
        ReliefHub hub(path);
        if (demoMode) {
            demo(hub);
            return 0;
        }
        PriorityDispatchQueue<Mission, UrgentMedicalFirst> queue;
        while (true) {
            std::cout << "\nERL-HUB | Earthquake Relief Logistics\n1 Inventory  2 Fleet  3 Restock "
                         " 4 Queue mission\n5 Dispatch next  6 Audit log  7 Seed supplies  8 "
                         "Statistics  0 Exit\n";
            if (!std::cin.good())
                break;
            try {
                int choice = number<int>("Choice: ");
                if (choice == 0)
                    break;
                if (choice == 1)
                    inventory(hub);
                else if (choice == 2)
                    fleet(hub);
                else if (choice == 3) {
                    ReliefCargo c;
                    std::istringstream in(
                        line("Cargo: SKU units unitKg unitM3 (quote names with spaces): "));
                    if (!(in >> c))
                        throw std::invalid_argument("Invalid cargo");
                    in >> std::ws;
                    if (!in.eof())
                        throw std::invalid_argument("Trailing cargo input");
                    hub.restock(c);
                    std::cout << "Stock saved.\n";
                } else if (choice == 4) {
                    Mission m;
                    m.id = number<int>("Unique mission ID: ");
                    m.vehicleId = line("Vehicle ID: ");
                    updateMissionPriority(m, number<int>("Priority 1..5: "));
                    m.route.destination.x = number<double>("Destination x (km): ");
                    m.route.destination.y = number<double>("Destination y (km): ");
                    m.route.roadOpen = flag("Road open 0/1: ");
                    m.route.waterOpen = flag("Water open 0/1: ");
                    m.route.weatherClear = flag("Flight weather clear 0/1: ");
                    int mode = number<int>("Allocation: 1 volume / 2 medical: ");
                    if (mode == 1) {
                        auto sku = line("SKU: ");
                        double v = number<double>("Volume budget m3: ");
                        allocateSupply(m.cargo, hub.depot(), sku, v);
                    } else if (mode == 2) {
                        int patients = number<int>("Patients: ");
                        allocateSupply(m.cargo, hub.depot(), "medicine", patients, m.priority);
                        if (flag("Commander override to priority 5? 0/1: "))
                            IncidentCommander{}.triageOverride(hub.vehicle(m.vehicleId), m, 5);
                    } else
                        throw std::invalid_argument("Unknown allocation mode");
                    hub.vehicle(m.vehicleId).validateDispatch(m);
                    queue.push(m);
                    printManifest(std::cout, m.cargo);
                    std::cout << "Queued; inventory is rechecked on departure.\n";
                } else if (choice == 5) {
                    auto m = queue.top();
                    std::cout << "Next mission " << m.id << '\n';
                    auto agency = line("Dispatching agency: ");
                    try {
                        hub.dispatch(m, agency);
                        queue.pop();
                        std::cout << "Dispatch saved.\n";
                    } catch (const std::exception &e) {
                        std::cout << e.what() << '\n';
                        if (flag("Remove failed mission? 0/1: "))
                            queue.pop();
                    }
                } else if (choice == 6) {
                    for (const auto &r : hub.records()) {
                        std::cout << "Mission " << r.mission.id << " | " << r.agency << " | "
                                  << r.mission.vehicleId << " | fuel " << r.fuelCost << '\n';
                        printManifest(std::cout, r.mission.cargo);
                    }
                } else if (choice == 7) {
                    seed(hub);
                    std::cout << "Supplies added and saved.\n";
                } else if (choice == 8)
                    std::cout << "Operational vehicles: " << TransportVehicle::operationalVehicles()
                              << " | dispatched tonnes: " << ReliefHub::dispatchedTonnes()
                              << " | queued: " << queue.size() << '\n';
                else
                    std::cout << "Unknown choice.\n";
            } catch (const std::exception &e) {
                std::cerr << "Error: " << e.what() << '\n';
                if (std::cin.eof())
                    break;
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Startup/operation failed: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
