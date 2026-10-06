#include "core/MauSac.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

int MauSac::chuanHoa(int val) {
    return std::max(0, std::min(255, val));
}

MauSac::MauSac() : r(0), g(0), b(0), a(255) {}

MauSac::MauSac(int red, int green, int blue, int alpha)
    : r(chuanHoa(red)), g(chuanHoa(green)), b(chuanHoa(blue)), a(chuanHoa(alpha)) {}

int MauSac::getR() const { return r; }
int MauSac::getG() const { return g; }
int MauSac::getB() const { return b; }
int MauSac::getA() const { return a; }

void MauSac::setR(int red) { r = chuanHoa(red); }
void MauSac::setG(int green) { g = chuanHoa(green); }
void MauSac::setB(int blue) { b = chuanHoa(blue); }
void MauSac::setA(int alpha) { a = chuanHoa(alpha); }

std::string MauSac::toHex(bool baoGomAlpha) const {
    std::ostringstream oss;
    oss << "#"
        << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << r
        << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << g
        << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << b;
    if (baoGomAlpha) {
        oss << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << a;
    }
    return oss.str();
}

bool MauSac::operator==(const MauSac& other) const {
    return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool MauSac::operator!=(const MauSac& other) const {
    return !(*this == other);
}

MauSac MauSac::Den() { return MauSac(0, 0, 0, 255); }
MauSac MauSac::Trang() { return MauSac(255, 255, 255, 255); }
MauSac MauSac::Do() { return MauSac(255, 0, 0, 255); }
MauSac MauSac::XanhLa() { return MauSac(0, 255, 0, 255); }
MauSac MauSac::XanhDuong() { return MauSac(0, 0, 255, 255); }