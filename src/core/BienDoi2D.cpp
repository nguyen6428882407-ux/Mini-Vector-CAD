#include "core/BienDoi2D.h"
#include "core/MathUtils.h"
#include <cmath>
#include <stdexcept>

BienDoi2D::BienDoi2D() {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            m_[i][j] = (i == j) ? 1.0 : 0.0;
        }
    }
}

BienDoi2D BienDoi2D::tinhTien(double dx, double dy) {
    BienDoi2D res;
    res.m_[0][2] = dx;
    res.m_[1][2] = dy;
    return res;
}

BienDoi2D BienDoi2D::xoay(double gocDo, const Diem2D& tam) {
    double rad = MathUtils::tuDoSangRadian(gocDo);
    double c = std::cos(rad);
    double s = std::sin(rad);

    double gocMod = std::fmod(std::fmod(gocDo, 360.0) + 360.0, 360.0);
    if (MathUtils::bangNhau(gocMod, 0.0) || MathUtils::bangNhau(gocMod, 360.0)) {
        c = 1.0; s = 0.0;
    } else if (MathUtils::bangNhau(gocMod, 90.0)) {
        c = 0.0; s = 1.0;
    } else if (MathUtils::bangNhau(gocMod, 180.0)) {
        c = -1.0; s = 0.0;
    } else if (MathUtils::bangNhau(gocMod, 270.0)) {
        c = 0.0; s = -1.0;
    }

    double tx = tam.getX();
    double ty = tam.getY();

    BienDoi2D res;
    res.m_[0][0] = c;
    res.m_[0][1] = -s;
    res.m_[0][2] = tx - c * tx + s * ty;
    res.m_[1][0] = s;
    res.m_[1][1] = c;
    res.m_[1][2] = ty - s * tx - c * ty;
    return res;
}

BienDoi2D BienDoi2D::tiLeDeu(double k, const Diem2D& tam) {
    if (std::abs(k) < MathUtils::EPS || !std::isfinite(k)) {
        throw std::invalid_argument("He so ti le phai khac 0 va huu han.");
    }

    double tx = tam.getX();
    double ty = tam.getY();

    BienDoi2D res;
    res.m_[0][0] = k;
    res.m_[0][2] = tx * (1.0 - k);
    res.m_[1][1] = k;
    res.m_[1][2] = ty * (1.0 - k);
    return res;
}

Diem2D BienDoi2D::apDung(const Diem2D& p) const {
    double x = p.getX();
    double y = p.getY();
    double nx = m_[0][0] * x + m_[0][1] * y + m_[0][2];
    double ny = m_[1][0] * x + m_[1][1] * y + m_[1][2];
    return Diem2D(nx, ny);
}

double BienDoi2D::heSoTiLe() const {
    double det = m_[0][0] * m_[1][1] - m_[0][1] * m_[1][0];
    if (!std::isfinite(det)) return std::nan("");
    return std::sqrt(std::abs(det));
}

BienDoi2D BienDoi2D::nghichDao() const {
    double det = m_[0][0] * m_[1][1] - m_[0][1] * m_[1][0];
    if (!std::isfinite(det) || std::abs(det) < MathUtils::EPS) {
        throw std::domain_error("Ma tran khong kha nghich.");
    }

    BienDoi2D inv;
    inv.m_[0][0] = m_[1][1] / det;
    inv.m_[0][1] = -m_[0][1] / det;
    inv.m_[0][2] = (m_[0][1] * m_[1][2] - m_[0][2] * m_[1][1]) / det;
    inv.m_[1][0] = -m_[1][0] / det;
    inv.m_[1][1] = m_[0][0] / det;
    inv.m_[1][2] = (m_[0][2] * m_[1][0] - m_[0][0] * m_[1][2]) / det;
    inv.m_[2][0] = 0.0;
    inv.m_[2][1] = 0.0;
    inv.m_[2][2] = 1.0;

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (!std::isfinite(inv.m_[i][j])) {
                throw std::domain_error("Nghich dao ma tran bi tran so.");
            }
        }
    }
    return inv;
}

BienDoi2D BienDoi2D::operator*(const BienDoi2D& o) const {
    BienDoi2D res;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            double sum = 0.0;
            for (int k = 0; k < 3; ++k) {
                sum += m_[i][k] * o.m_[k][j];
            }
            res.m_[i][j] = sum;
        }
    }
    return res;
}
