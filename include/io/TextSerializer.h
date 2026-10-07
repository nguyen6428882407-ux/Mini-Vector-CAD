#pragma once
#include <string>
#include <iosfwd>

class BangVeCanvas;

class TextSerializer {
public:
    static bool ghiFile(const BangVeCanvas& canvas, const std::string& duongDan);
    static bool docFile(BangVeCanvas& canvas, const std::string& duongDan);
    static void serialize(const BangVeCanvas& canvas, std::ostream& os);
    static void deserialize(BangVeCanvas& canvas, std::istream& is);
    static void inBangVe(const BangVeCanvas& canvas, std::ostream& os);
};
