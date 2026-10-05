#include "shapes/HinhHoc2D.h"

int HinhHoc2D::demId_ = 0;

HinhHoc2D::HinhHoc2D()
    : id_(++demId_),
      vien_(0, 0, 0, 1.0),
      nen_(255, 255, 255, 0.0),
      doDayNet_(1.0) {}

HinhHoc2D::HinhHoc2D(const HinhHoc2D& other)
    : id_(++demId_),
      vien_(other.vien_),
      nen_(other.nen_),
      doDayNet_(other.doDayNet_) {}

HinhHoc2D::~HinhHoc2D() {}

int HinhHoc2D::getId() const { return id_; }
void HinhHoc2D::setMauVien(const MauSac& m) { vien_ = m; }
void HinhHoc2D::setMauNen(const MauSac& m) { nen_ = m; }
void HinhHoc2D::setDoDayNet(double d) { doDayNet_ = d; }

// Getter style: để SvgExporter, TextSerializer đọc mà không truy cập protected.
const MauSac& HinhHoc2D::getMauVien()  const { return vien_; }
const MauSac& HinhHoc2D::getMauNen()   const { return nen_; }
double         HinhHoc2D::getDoDayNet() const { return doDayNet_; }

