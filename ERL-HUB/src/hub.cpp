#include "erl/hub.hpp"
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <unordered_set>
namespace ReliefOps {
namespace Logistics {
namespace {
// Explicit little-endian integers; never write raw objects containing pointers.
void write32(std::ostream &o, std::uint32_t n) {
    for (int j = 0; j < 4; ++j)
        o.put(static_cast<char>((n >> (8 * j)) & 255));
}
std::uint32_t read32(std::istream &i) {
    std::uint32_t n = 0;
    for (int j = 0; j < 4; ++j) {
        int c = i.get();
        if (c == EOF)
            throw std::runtime_error("Truncated binary record");
        n |= static_cast<std::uint32_t>(c) << (8 * j);
    }
    return n;
}
std::uint32_t checksum(const std::string &s) {
    std::uint32_t h = 2166136261u;
    for (unsigned char c : s) {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}
std::string encode(const HubState &s) {
    std::ostringstream o;
    o.precision(17);
    auto items = s.depot.items();
    o << items.size() << '\n';
    for (const auto &c : items)
        o << c << '\n';
    o << s.records.size() << '\n';
    for (const auto &r : s.records) {
        const auto &m = r.mission;
        o << m.id << ' ' << m.priority << ' ' << std::quoted(m.vehicleId) << ' '
          << m.route.destination.x << ' ' << m.route.destination.y << ' ' << m.route.roadOpen << ' '
          << m.route.waterOpen << ' ' << m.route.weatherClear << ' ' << r.fuelCost << ' '
          << std::quoted(r.agency) << '\n'
          << m.cargo;
    }
    return o.str();
}
HubState decode(const std::string &payload) {
    std::istringstream i(payload);
    HubState s;
    std::size_t n;
    std::unordered_set<std::string> stockIds;
    if (!(i >> n) || n > 10000)
        throw std::runtime_error("Invalid stock count");
    for (std::size_t j = 0; j < n; ++j) {
        Inventory::ReliefCargo c;
        if (!(i >> c))
            throw std::runtime_error("Invalid stock record");
        if (!stockIds.insert(c.sku()).second)
            throw std::runtime_error("Duplicate stock record");
        s.depot += c;
    }
    if (!(i >> n) || n > 100000)
        throw std::runtime_error("Invalid audit count");
    for (std::size_t j = 0; j < n; ++j) {
        AuditRecord r;
        auto &m = r.mission;
        if (!(i >> m.id >> m.priority >> std::quoted(m.vehicleId) >> m.route.destination.x >>
              m.route.destination.y >> m.route.roadOpen >> m.route.waterOpen >>
              m.route.weatherClear >> r.fuelCost >> std::quoted(r.agency) >> m.cargo) ||
            !std::isfinite(r.fuelCost) || r.fuelCost < 0 || r.agency.empty() ||
            r.agency.size() > 100)
            throw std::runtime_error("Invalid audit record");
        s.records.push_back(r);
    }
    i >> std::ws;
    if (!i.eof())
        throw std::runtime_error("Unexpected trailing payload");
    return s;
}
} // namespace
HubState BinaryStore::load() const {
    if (!std::filesystem::exists(path_))
        return {};
    std::fstream in(path_, std::ios::in | std::ios::binary);
    if (!in)
        throw std::runtime_error("Cannot open state file");
    char magic[8];
    in.read(magic, 8);
    if (!in || std::string(magic, 8) != "ERLHUB01")
        throw std::runtime_error("Invalid state header/version");
    auto length = read32(in), expected = read32(in);
    if (length > 16 * 1024 * 1024)
        throw std::runtime_error("State exceeds size limit");
    std::string payload(length, '\0');
    if (length)
        in.read(&payload[0], length);
    if (!in || in.peek() != EOF || checksum(payload) != expected)
        throw std::runtime_error("Corrupt or truncated state checksum");
    return decode(payload);
}
void BinaryStore::save(const HubState &s, bool fail) const {
    std::string payload = encode(s);
    if (payload.size() > 16 * 1024 * 1024)
        throw std::runtime_error("State exceeds size limit");
    const std::string tmp = path_ + ".tmp";
    try {
        std::fstream out(tmp, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out)
            throw std::runtime_error("Cannot create transaction file");
        out.write("ERLHUB01", 8);
        write32(out, static_cast<std::uint32_t>(payload.size()));
        write32(out, checksum(payload));
        out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
        out.flush();
        if (!out)
            throw std::runtime_error("State write failed");
        out.close();
        if (out.fail())
            throw std::runtime_error("State close failed");
        BinaryStore(tmp).load(); // Read back and verify before the commit point.
        if (fail)
            throw std::runtime_error("Injected failure before atomic commit");
        if (std::rename(tmp.c_str(), path_.c_str()) != 0)
            throw std::runtime_error("Atomic replacement failed; original retained");
    } catch (...) {
        std::remove(tmp.c_str());
        throw;
    }
}
double ReliefHub::dispatchedTonnes_ = 0;
ReliefHub::ReliefHub(std::string path) : store_(std::move(path)), fleet_(makeFleet()) {
    state_ = store_.load();
    double total = 0;
    std::vector<int> ids;
    for (const auto &r : state_.records) {
        if (std::find(ids.begin(), ids.end(), r.mission.id) != ids.end())
            throw std::runtime_error("Duplicate saved mission ID");
        ids.push_back(r.mission.id);
        auto &v = vehicle(r.mission.vehicleId);
        double cost = v.validateDispatch(r.mission);
        if (std::abs(cost - r.fuelCost) > 1e-9)
            throw std::runtime_error("Audit route cost mismatch");
        v.completeDispatch(r.mission.route, cost);
        total += tonnes(r.mission.cargo.weight());
    }
    dispatchedTonnes_ = total;
}
std::vector<ReliefVehicle *> ReliefHub::fleet() const {
    std::vector<ReliefVehicle *> out;
    for (const auto &v : fleet_)
        out.push_back(v.get());
    return out;
}
ReliefVehicle &ReliefHub::vehicle(const std::string &id) const {
    for (const auto &v : fleet_)
        if (v->id() == id)
            return *v;
    throw std::invalid_argument("Unknown vehicle ID");
}
void ReliefHub::restock(const Inventory::ReliefCargo &c, bool fail) {
    HubState next = state_;
    Inventory::restock(next.depot, c);
    store_.save(next, fail);
    state_.depot.swap(next.depot);
}
void ReliefHub::dispatch(const Mission &m, const std::string &agency, bool fail) {
    if (agency.empty() || agency.size() > 100)
        throw std::invalid_argument("Agency required (1..100 characters)");
    for (const auto &r : state_.records)
        if (r.mission.id == m.id)
            throw std::invalid_argument("Duplicate mission ID");
    auto &v = vehicle(m.vehicleId);
    double cost = v.validateDispatch(m);
    HubState next = state_;
    for (std::size_t j = 0; j < m.cargo.size(); ++j)
        next.depot -= m.cargo.at(j);
    next.records.push_back({m, cost, agency});
    store_.save(next, fail);
    // All operations after persistent commit are nonthrowing.
    state_.depot.swap(next.depot);
    state_.records.swap(next.records);
    v.completeDispatch(m.route, cost);
    dispatchedTonnes_ += tonnes(m.cargo.weight());
}
} // namespace Logistics
} // namespace ReliefOps
