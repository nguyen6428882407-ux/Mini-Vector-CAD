#include "core/Diem2D.h"
#include <cmath>

Diem2D::Diem2D() : x(0.0), y(0.0) {}
Diem2D::Diem2D(double x, double y) : x(x), y(y) {}

double Diem2D::getX() const { return x; }
double Diem2D::getY() const { return y; }
void Diem2D::setX(double newX) { x = newX; }
void Diem2D::setY(double newY) { y = newY; }

double Diem2D::khoangCach(const Diem2D& other) const {
    double dx = x - other.x;
    double dy = y - other.y;
    return std::sqrt(dx * dx + dy * dy);
}

double Diem2D::khoangCach(const Diem2D& p1, const Diem2D& p2) {
    return p1.khoangCach(p2);
}

Diem2D Diem2D::operator+(const Diem2D& other) const {
    return Diem2D(x + other.x, y + other.y);
}

Diem2D Diem2D::operator-(const Diem2D& other) const {
    return Diem2D(x - other.x, y - other.y);
}

Diem2D Diem2D::operator*(double k) const {
    return Diem2D(x * k, y * k);
}

bool Diem2D::operator==(const Diem2D& other) const {
    return MathUtils::bangNhau(x, other.x) && MathUtils::bangNhau(y, other.y);
}

bool Diem2D::operator!=(const Diem2D& other) const {
    return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, const Diem2D& p) {
    os << "(" << p.x << ", " << p.y << ")";
    return os;
}
