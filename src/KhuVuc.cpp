#include "../include/KhuVuc.h"

KhuVuc::KhuVuc() : maKhuVuc("KV3"), tenKhuVuc("Khu vuc 3 (Khong uu tien)"), diemUuTien(0.0) {}

KhuVuc::KhuVuc(const string& ma, const string& ten, double diem)
    : maKhuVuc(ma), tenKhuVuc(ten), diemUuTien(diem) {}

KhuVuc::KhuVuc(const KhuVuc& other)
    : maKhuVuc(other.maKhuVuc), tenKhuVuc(other.tenKhuVuc), diemUuTien(other.diemUuTien) {}

KhuVuc::~KhuVuc() {}

string KhuVuc::getMaKhuVuc() const { return maKhuVuc; }
string KhuVuc::getTenKhuVuc() const { return tenKhuVuc; }
double KhuVuc::getDiemUuTien() const { return diemUuTien; }

void KhuVuc::setMaKhuVuc(const string& ma) { maKhuVuc = ma; }
void KhuVuc::setTenKhuVuc(const string& ten) { tenKhuVuc = ten; }
void KhuVuc::setDiemUuTien(double diem) { diemUuTien = diem; }

KhuVuc& KhuVuc::operator=(const KhuVuc& other) {
    if (this != &other) {
        maKhuVuc = other.maKhuVuc;
        tenKhuVuc = other.tenKhuVuc;
        diemUuTien = other.diemUuTien;
    }
    return *this;
}

bool KhuVuc::operator==(const KhuVuc& other) const {
    return (maKhuVuc == other.maKhuVuc);
}
