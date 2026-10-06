#ifndef DIEM_2D_H
#define DIEM_2D_H

#include "MathUtils.h"
#include <iostream>

class Diem2D {
private:
    double x;
    double y;

public:
    Diem2D();
    Diem2D(double x, double y);

    double getX() const;
    double getY() const;
    void setX(double x);
    void setY(double y);

    double khoangCach(const Diem2D& other) const;
    static double khoangCach(const Diem2D& p1, const Diem2D& p2);

    Diem2D operator+(const Diem2D& other) const;
    Diem2D operator-(const Diem2D& other) const;
    Diem2D operator*(double k) const;

    bool operator==(const Diem2D& other) const;
    bool operator!=(const Diem2D& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Diem2D& p);
};

#endif // DIEM_2D_H