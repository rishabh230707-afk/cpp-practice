#include "erl/inventory.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <istream>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <utility>
namespace ReliefOps {
namespace Inventory {
ReliefCargo::ReliefCargo(std::string sku, int n, double kg, double v)
    : sku_(std::move(sku)), units_(n), unitKg_(kg), unitVolume_(v) {
    if (sku_.empty() || sku_.size() > 100 || n < 0 || !std::isfinite(kg) || kg <= 0 || kg > 1e6 ||
        !std::isfinite(v) || v <= 0 || v > 1e6)
        throw std::invalid_argument("Invalid cargo fields");
}
bool ReliefCargo::operator==(const ReliefCargo &b) const noexcept {
    return sku_ == b.sku_ && units_ == b.units_ && unitKg_ == b.unitKg_ &&
           unitVolume_ == b.unitVolume_;
}
std::ostream &operator<<(std::ostream &o, const ReliefCargo &c) {
    auto precision = o.precision();
    o << std::quoted(c.sku_) << ' ' << c.units_ << ' ' << std::setprecision(17) << c.unitKg_ << ' '
      << c.unitVolume_;
    o.precision(precision);
    return o;
}
std::istream &operator>>(std::istream &i, ReliefCargo &c) {
    std::string s;
    int n;
    double kg, v;
    if (i >> std::quoted(s) >> n >> kg >> v) {
        try {
            c = ReliefCargo(s, n, kg, v);
        } catch (const std::invalid_argument &) {
            i.setstate(std::ios::failbit);
        }
    }
    return i;
}
Manifest::Manifest(std::size_t c) : items_(nullptr), size_(0), capacity_(c) {
    if (c > 10000)
        throw std::length_error("Manifest capacity limit");
    items_ = new ReliefCargo[c];
}
Manifest::Manifest(const Manifest &b) : Manifest(b.capacity_) {
    if (b.size_)
        std::copy(b.items_, b.items_ + b.size_, items_);
    size_ = b.size_;
}
Manifest::Manifest(Manifest &&b) noexcept
    : items_(b.items_), size_(b.size_), capacity_(b.capacity_) {
    b.items_ = nullptr;
    b.size_ = b.capacity_ = 0;
}
Manifest &Manifest::operator=(Manifest b) {
    swap(b);
    return *this;
}
Manifest::~Manifest() {
    delete[] items_;
}
void Manifest::swap(Manifest &b) noexcept {
    using std::swap;
    swap(items_, b.items_);
    swap(size_, b.size_);
    swap(capacity_, b.capacity_);
}
void Manifest::add(const ReliefCargo &c) {
    if (c.units() == 0)
        throw std::invalid_argument("Manifest cargo must contain units");
    for (std::size_t j = 0; j < size_; ++j)
        if (items_[j] == c || items_[j].sku() == c.sku())
            throw std::invalid_argument("Duplicate cargo SKU");
    if (size_ == capacity_) {
        Manifest next(capacity_ ? capacity_ * 2 : 4);
        for (std::size_t j = 0; j < size_; ++j)
            next.add(items_[j]);
        next.add(c);
        swap(next);
        return;
    }
    items_[size_] = c;
    ++size_;
}
const ReliefCargo &Manifest::at(std::size_t j) const {
    if (j >= size_)
        throw std::out_of_range("Manifest index");
    return items_[j];
}
double Manifest::weight() const noexcept {
    double n = 0;
    for (std::size_t j = 0; j < size_; ++j)
        n += items_[j].weight();
    return n;
}
double Manifest::volume() const noexcept {
    double n = 0;
    for (std::size_t j = 0; j < size_; ++j)
        n += items_[j].volume();
    return n;
}
bool Manifest::operator==(const Manifest &b) const noexcept {
    if (size_ != b.size_)
        return false;
    for (std::size_t j = 0; j < size_; ++j) {
        bool found = false;
        for (std::size_t k = 0; k < b.size_; ++k)
            if (items_[j] == b.items_[k])
                found = true;
        if (!found)
            return false;
    }
    return true;
}
std::ostream &operator<<(std::ostream &o, const Manifest &m) {
    o << m.size_ << '\n';
    for (std::size_t j = 0; j < m.size_; ++j)
        o << m.items_[j] << '\n';
    return o;
}
std::istream &operator>>(std::istream &i, Manifest &m) {
    std::size_t n;
    if (!(i >> n))
        return i;
    if (n > 10000) {
        i.setstate(std::ios::failbit);
        return i;
    }
    Manifest tmp(n);
    for (std::size_t j = 0; j < n; ++j) {
        ReliefCargo c;
        if (!(i >> c))
            return i;
        try {
            tmp.add(c);
        } catch (const std::invalid_argument &) {
            i.setstate(std::ios::failbit);
            return i;
        }
    }
    m.swap(tmp);
    return i;
}
DistributionDepot &DistributionDepot::operator+=(const ReliefCargo &c) {
    auto it = stock_.find(c.sku());
    if (it == stock_.end()) {
        stock_.emplace(c.sku(), c);
        return *this;
    }
    const auto &old = it->second;
    if (old.unitKg() != c.unitKg() || old.unitVolume() != c.unitVolume())
        throw std::invalid_argument("SKU dimensions disagree");
    if (c.units() > std::numeric_limits<int>::max() - old.units())
        throw std::overflow_error("Stock overflow");
    it->second = ReliefCargo(c.sku(), old.units() + c.units(), c.unitKg(), c.unitVolume());
    return *this;
}
DistributionDepot &DistributionDepot::operator-=(const ReliefCargo &c) {
    auto it = stock_.find(c.sku());
    if (it == stock_.end() || it->second.units() < c.units())
        throw std::invalid_argument("Insufficient inventory");
    if (it->second.unitKg() != c.unitKg() || it->second.unitVolume() != c.unitVolume())
        throw std::invalid_argument("SKU dimensions disagree");
    it->second = ReliefCargo(c.sku(), it->second.units() - c.units(), c.unitKg(), c.unitVolume());
    return *this;
}
int DistributionDepot::available(const std::string &s) const {
    auto it = stock_.find(s);
    return it == stock_.end() ? 0 : it->second.units();
}
const ReliefCargo &DistributionDepot::item(const std::string &s) const {
    return stock_.at(s);
}
std::vector<ReliefCargo> DistributionDepot::items() const {
    std::vector<ReliefCargo> out;
    for (const auto &p : stock_)
        out.push_back(p.second);
    std::sort(out.begin(), out.end(),
              [](const ReliefCargo &a, const ReliefCargo &b) { return a.sku() < b.sku(); });
    return out;
}
void DistributionDepot::swap(DistributionDepot &b) noexcept {
    stock_.swap(b.stock_);
}
DistributionDepot &restock(DistributionDepot &d, const ReliefCargo &c) {
    return d += c;
}
Manifest &allocateSupply(Manifest &m, const DistributionDepot &d, const std::string &s,
                         double volume) {
    if (!std::isfinite(volume) || volume <= 0)
        throw std::invalid_argument("Invalid volume budget");
    const auto &c = d.item(s);
    int n = static_cast<int>(
        std::min(static_cast<double>(c.units()), std::floor(volume / c.unitVolume())));
    m.add(ReliefCargo(s, n, c.unitKg(), c.unitVolume()));
    return m;
}
Manifest &allocateSupply(Manifest &m, const DistributionDepot &d, const std::string &s,
                         int patients, int priority) {
    if (s != "medicine" || patients <= 0 || priority < 1 || priority > 5)
        throw std::invalid_argument(
            "Medical allocation requires medicine, patients>0, priority 1..5");
    const auto &c = d.item(s);
    int factor = priority >= 4 ? 2 : 1;
    if (patients > c.units() / factor)
        throw std::invalid_argument("Insufficient triage medicine");
    m.add(ReliefCargo(s, patients * factor, c.unitKg(), c.unitVolume()));
    return m;
}
std::ostream &manifestHeader(std::ostream &o) {
    return o << "SKU                    UNITS       KG       M3\n"
             << "-----------------------------------------------\n";
}
void printManifest(std::ostream &o, const Manifest &m) {
    auto flags = o.flags();
    auto p = o.precision();
    o << manifestHeader;
    for (std::size_t j = 0; j < m.size(); ++j) {
        const auto &c = m.at(j);
        o << std::left << std::setw(22) << c.sku() << std::right << std::setw(6) << c.units()
          << std::fixed << std::setprecision(2) << std::setw(9) << c.weight() << std::setw(9)
          << c.volume() << '\n';
    }
    o.flags(flags);
    o.precision(p);
}
} // namespace Inventory
} // namespace ReliefOps
