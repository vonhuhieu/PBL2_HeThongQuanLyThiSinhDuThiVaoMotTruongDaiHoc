#include "../include/NganhXetTuyen.h"

NganhXetTuyen::NganhXetTuyen()
    : maNganh(""), tenNganh(""), toHopMon("A00"), chiTieu(0), diemChuan(20.0) {}

NganhXetTuyen::NganhXetTuyen(const string& ma, const string& ten, const string& toHop, int ct, double dc)
    : maNganh(ma), tenNganh(ten), toHopMon(toHop), chiTieu(ct), diemChuan(dc) {}

NganhXetTuyen::NganhXetTuyen(const NganhXetTuyen& other)
    : maNganh(other.maNganh), tenNganh(other.tenNganh), toHopMon(other.toHopMon),
      chiTieu(other.chiTieu), diemChuan(other.diemChuan) {}

NganhXetTuyen::~NganhXetTuyen() {}

string NganhXetTuyen::getMaNganh() const { return maNganh; }
string NganhXetTuyen::getTenNganh() const { return tenNganh; }
string NganhXetTuyen::getToHopMon() const { return toHopMon; }
int NganhXetTuyen::getChiTieu() const { return chiTieu; }
double NganhXetTuyen::getDiemChuan() const { return diemChuan; }

void NganhXetTuyen::setMaNganh(const string& ma) { maNganh = ma; }
void NganhXetTuyen::setTenNganh(const string& ten) { tenNganh = ten; }
void NganhXetTuyen::setToHopMon(const string& toHop) { toHopMon = toHop; }
void NganhXetTuyen::setChiTieu(int ct) { chiTieu = ct; }
void NganhXetTuyen::setDiemChuan(double dc) { diemChuan = dc; }

NganhXetTuyen& NganhXetTuyen::operator=(const NganhXetTuyen& other) {
    if (this != &other) {
        maNganh = other.maNganh;
        tenNganh = other.tenNganh;
        toHopMon = other.toHopMon;
        chiTieu = other.chiTieu;
        diemChuan = other.diemChuan;
    }
    return *this;
}

bool NganhXetTuyen::operator==(const NganhXetTuyen& other) const {
    return (maNganh == other.maNganh);
}
