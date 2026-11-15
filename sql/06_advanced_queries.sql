-- ==============================================================================
-- HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC - PBL2 (DE TAI 24)
-- File: 06_advanced_queries.sql
-- Muc tieu: Tap hop cac cau lenh truy van SQL nang cao cho bao cao & thong ke K26
-- Quy chuan: GEMINI.md - 100% lowerCamelCase
-- ==============================================================================

USE `pbl2_tuyensinh`;

-- 1. TRUY VAN KET HOP TOAN DIEN (Thong tin thi sinh + Que quan + Gioi tinh + Nganh & Khoa)
SELECT 
    ts.soBaoDanh                                        AS `So_Bao_Danh`,
    n.soCCCD                                            AS `So_CCCD`,
    n.hoTen                                             AS `Ho_Va_Ten`,
    n.ngaySinh                                          AS `Ngay_Sinh`,
    tt.tenTinh                                          AS `Que_Quan`,
    gt.tenGioiTinh                                      AS `Gioi_Tinh`,
    ts.diemToan                                         AS `Diem_Toan`,
    ts.diemLy                                           AS `Diem_Ly`,
    ts.diemHoa                                          AS `Diem_Hoa`,
    ROUND(ts.diemToan + ts.diemLy + ts.diemHoa, 2)     AS `Tong_Diem`,
    COALESCE(ts.maSinhVien, 'Chua cap')                 AS `Ma_Sinh_Vien`,
    COALESCE(ts.lopSinhHoat, 'Chua phan')               AS `Lop_Sinh_Hoat`,
    COALESCE(dmn.tenNganh, 'Chua trung tuyen')          AS `Nganh_Trung_Tuyen`,
    ts.nguyenVongTrungTuyen                             AS `NV_Trung_Tuyen`
FROM `thisinh` ts
JOIN `nguoi` n ON ts.soCCCD = n.soCCCD
JOIN `tinh_thanh` tt ON n.queQuan = tt.maTinh
JOIN `gioi_tinh` gt ON n.gioiTinh = gt.maGioiTinh
LEFT JOIN `danh_muc_nganh` dmn ON ts.nganhTrungTuyen = dmn.maNganh
ORDER BY `Tong_Diem` DESC, ts.diemToan DESC;

-- 2. TRUY VAN DANH SACH NGUYEN VONG CUA THI SINH
SELECT 
    nv.soBaoDanh,
    n.hoTen,
    nv.thuTuNV,
    dmn.maNganh,
    dmn.tenNganh,
    k.tenKhoa,
    nv.trangThai
FROM `nguyen_vong` nv
JOIN `thisinh` ts ON nv.soBaoDanh = ts.soBaoDanh
JOIN `nguoi` n ON ts.soCCCD = n.soCCCD
JOIN `danh_muc_nganh` dmn ON nv.maNganh = dmn.maNganh
JOIN `khoa` k ON dmn.maKhoa = k.maKhoa
ORDER BY nv.soBaoDanh ASC, nv.thuTuNV ASC;

-- 3. TRUY VAN DANH SACH TAN SINH VIEN TRUNG TUYEN CO MA SINH VIEN VA LOP SINH HOAT
SELECT 
    ts.maSinhVien                                       AS `Ma_Sinh_Vien_DUT`,
    n.hoTen                                             AS `Ho_Va_Ten`,
    ts.lopSinhHoat                                      AS `Lop_Sinh_Hoat`,
    dmn.tenNganh                                        AS `Nganh_Trung_Tuyen`,
    k.tenKhoa                                           AS `Khoa_Quan_Ly`,
    ROUND(ts.diemToan + ts.diemLy + ts.diemHoa, 2)     AS `Diem_Trung_Tuyen`,
    ts.nguyenVongTrungTuyen                             AS `Trung_Tuyen_NV`
FROM `thisinh` ts
JOIN `nguoi` n ON ts.soCCCD = n.soCCCD
JOIN `danh_muc_nganh` dmn ON ts.nganhTrungTuyen = dmn.maNganh
JOIN `khoa` k ON dmn.maKhoa = k.maKhoa
WHERE ts.maSinhVien IS NOT NULL
ORDER BY ts.lopSinhHoat ASC, ts.maSinhVien ASC;

-- 4. TRUY VAN THONG KE PHO DIEM 5 MUC
SELECT 
    phanKhuc,
    COUNT(*) AS soLuong,
    ROUND(COUNT(*) * 100.0 / (SELECT COUNT(*) FROM `thisinh`), 2) AS tyLePhanTram
FROM (
    SELECT 
        CASE 
            WHEN (diemToan + diemLy + diemHoa) >= 27.0 THEN 'Rat Cao (>= 27.0)'
            WHEN (diemToan + diemLy + diemHoa) >= 24.0 THEN 'Cao (24.0 - < 27.0)'
            WHEN (diemToan + diemLy + diemHoa) >= 21.0 THEN 'Kha (21.0 - < 24.0)'
            WHEN (diemToan + diemLy + diemHoa) >= 15.0 THEN 'Trung Binh (15.0 - < 21.0)'
            ELSE 'Thap (< 15.0)'
        END AS phanKhuc,
        CASE 
            WHEN (diemToan + diemLy + diemHoa) >= 27.0 THEN 1
            WHEN (diemToan + diemLy + diemHoa) >= 24.0 THEN 2
            WHEN (diemToan + diemLy + diemHoa) >= 21.0 THEN 3
            WHEN (diemToan + diemLy + diemHoa) >= 15.0 THEN 4
            ELSE 5
        END AS thuTu
    FROM `thisinh`
) AS bangPhanKhuc
GROUP BY phanKhuc, thuTu
ORDER BY thuTu ASC;

-- 5. TRUY VAN VINH DANH THU KHOA KHOA 2026
SELECT 
    ts.soBaoDanh,
    n.soCCCD,
    n.hoTen,
    n.ngaySinh,
    tt.tenTinh AS queQuan,
    ts.diemToan,
    ts.diemLy,
    ts.diemHoa,
    ROUND(ts.diemToan + ts.diemLy + ts.diemHoa, 2) AS tongDiem
FROM `thisinh` ts
JOIN `nguoi` n ON ts.soCCCD = n.soCCCD
JOIN `tinh_thanh` tt ON n.queQuan = tt.maTinh
WHERE (ts.diemToan + ts.diemLy + ts.diemHoa) = (
    SELECT MAX(diemToan + diemLy + diemHoa) FROM `thisinh` WHERE namTuyenSinh = 2026
);

-- 6. TRUY VAN TI LE NGANH DUOC NGUOI HOC DANG KY NHIEU NHAT (XEP HANG DO "HOT" CUA NGANH)
SELECT 
    dmn.maNganh,
    dmn.tenNganh,
    COUNT(nv.id) AS soLuotDangKy,
    COUNT(CASE WHEN nv.thuTuNV = 1 THEN 1 END) AS soLuotNV1
FROM `danh_muc_nganh` dmn
LEFT JOIN `nguyen_vong` nv ON dmn.maNganh = nv.maNganh
GROUP BY dmn.maNganh, dmn.tenNganh
ORDER BY soLuotNV1 DESC, soLuotDangKy DESC;
