#ifndef MAU_SAC_H
#define MAU_SAC_H

#include <string>

class MauSac {
private:
    int r;
    int g;
    int b;
    int a;

    static int chuanHoa(int val);

public:
    MauSac();
    MauSac(int red, int green, int blue, int alpha = 255);

    int getR() const;
    int getG() const;
    int getB() const;
    int getA() const;

    void setR(int red);
    void setG(int green);
    void setB(int blue);
    void setA(int alpha);

    std::string toHex(bool baoGomAlpha = true) const;

    bool operator==(const MauSac& other) const;
    bool operator!=(const MauSac& other) const;

    static MauSac Den();
    static MauSac Trang();
    static MauSac Do();
    static MauSac XanhLa();
    static MauSac XanhDuong();
};

#endif // MAU_SAC_H