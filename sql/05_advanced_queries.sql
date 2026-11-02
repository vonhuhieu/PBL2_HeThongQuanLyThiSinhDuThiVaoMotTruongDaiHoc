-- ==============================================================================
-- HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC - PBL2 (DE TAI 24)
-- File: 05_advanced_queries.sql
-- Muc tieu: Tap hop cac cau lenh truy van SQL nang cao cho bao cao & thong ke
-- ==============================================================================

USE `pbl2_tuyensinh`;

-- 1. TRUY VAN KET HOP 4 BANG (Danh sach thi sinh day du thong tin co giai ma Danh muc)
SELECT 
    ts.sbd                      AS `So_Bao_Danh`,
    n.so_cccd                   AS `So_CCCD`,
    n.ho_ten                    AS `Ho_Va_Ten`,
    n.ngay_sinh                 AS `Ngay_Sinh`,
    tt.ten_tinh                 AS `Que_Quan`,
    gt.ten_gioi_tinh            AS `Gioi_Tinh`,
    ts.diem_toan                AS `Diem_Toan`,
    ts.diem_ly                  AS `Diem_Ly`,
    ts.diem_hoa                 AS `Diem_Hoa`,
    ROUND(ts.diem_toan + ts.diem_ly + ts.diem_hoa, 2) AS `Tong_Diem`
FROM `thisinh` ts
JOIN `nguoi` n ON ts.so_cccd = n.so_cccd
JOIN `tinh_thanh` tt ON n.que_quan = tt.ma_tinh
JOIN `gioi_tinh` gt ON n.gioi_tinh = gt.ma_gioi_tinh
ORDER BY `Tong_Diem` DESC, ts.diem_toan DESC;

-- 2. TRUY VAN XET TUYEN DONG THEO DIEM CHUAN & DIEM LIET TU BANG CAU HINH
SELECT 
    ts.sbd,
    n.ho_ten,
    tt.ten_tinh AS que_quan,
    ROUND(ts.diem_toan + ts.diem_ly + ts.diem_hoa, 2) AS tong_diem,
    CASE 
        WHEN (ts.diem_toan + ts.diem_ly + ts.diem_hoa) >= CAST(ch_dc.gia_tri AS DECIMAL(5,2))
             AND ts.diem_toan > CAST(ch_dl.gia_tri AS DECIMAL(5,2))
             AND ts.diem_ly   > CAST(ch_dl.gia_tri AS DECIMAL(5,2))
             AND ts.diem_hoa  > CAST(ch_dl.gia_tri AS DECIMAL(5,2))
        THEN 'TRUNG TUYEN'
        ELSE 'Khong dat'
    END AS ket_qua_xet_tuyen
FROM `thisinh` ts
JOIN `nguoi` n ON ts.so_cccd = n.so_cccd
JOIN `tinh_thanh` tt ON n.que_quan = tt.ma_tinh
CROSS JOIN (SELECT `gia_tri` FROM `cau_hinh` WHERE `khoa` = 'diem_chuan') ch_dc
CROSS JOIN (SELECT `gia_tri` FROM `cau_hinh` WHERE `khoa` = 'diem_liet') ch_dl
ORDER BY tong_diem DESC;

-- 3. TRUY VAN THONG KE PHO DIEM (GROUP BY PHAN KHUC DIEM)
SELECT 
    phan_khuc,
    COUNT(*) AS so_luong,
    ROUND(COUNT(*) * 100.0 / (SELECT COUNT(*) FROM `thisinh`), 2) AS ty_le_phan_tram
FROM (
    SELECT 
        CASE 
            WHEN (diem_toan + diem_ly + diem_hoa) >= 27.0 THEN 'Rat Cao (>= 27.0)'
            WHEN (diem_toan + diem_ly + diem_hoa) >= 24.0 THEN 'Cao (24.0 - < 27.0)'
            WHEN (diem_toan + diem_ly + diem_hoa) >= 21.0 THEN 'Kha (21.0 - < 24.0)'
            WHEN (diem_toan + diem_ly + diem_hoa) >= 15.0 THEN 'Trung Binh (15.0 - < 21.0)'
            ELSE 'Thap (< 15.0)'
        END AS phan_khuc,
        CASE 
            WHEN (diem_toan + diem_ly + diem_hoa) >= 27.0 THEN 1
            WHEN (diem_toan + diem_ly + diem_hoa) >= 24.0 THEN 2
            WHEN (diem_toan + diem_ly + diem_hoa) >= 21.0 THEN 3
            WHEN (diem_toan + diem_ly + diem_hoa) >= 15.0 THEN 4
            ELSE 5
        END AS thu_tu
    FROM `thisinh`
) AS bang_phan_khuc
GROUP BY phan_khuc, thu_tu
ORDER BY thu_tu ASC;

-- 4. TRUY VAN VINH DANH THU KHOA KY THI
SELECT 
    ts.sbd,
    n.so_cccd,
    n.ho_ten,
    n.ngay_sinh,
    tt.ten_tinh AS que_quan,
    ts.diem_toan,
    ts.diem_ly,
    ts.diem_hoa,
    ROUND(ts.diem_toan + ts.diem_ly + ts.diem_hoa, 2) AS tong_diem
FROM `thisinh` ts
JOIN `nguoi` n ON ts.so_cccd = n.so_cccd
JOIN `tinh_thanh` tt ON n.que_quan = tt.ma_tinh
WHERE (ts.diem_toan + ts.diem_ly + ts.diem_hoa) = (
    SELECT MAX(diem_toan + diem_ly + diem_hoa) FROM `thisinh`
);

-- 5. TRUY VAN THONG KE THI SINH THEO TINH THANH
SELECT 
    tt.ma_tinh,
    tt.ten_tinh,
    COUNT(ts.sbd) AS so_thi_sinh,
    ROUND(AVG(ts.diem_toan + ts.diem_ly + ts.diem_hoa), 2) AS diem_trung_binh,
    MAX(ts.diem_toan + ts.diem_ly + ts.diem_hoa) AS diem_cao_nhat
FROM `tinh_thanh` tt
LEFT JOIN `nguoi` n ON tt.ma_tinh = n.que_quan
LEFT JOIN `thisinh` ts ON n.so_cccd = ts.so_cccd
GROUP BY tt.ma_tinh, tt.ten_tinh
HAVING so_thi_sinh > 0
ORDER BY so_thi_sinh DESC, diem_trung_binh DESC;
