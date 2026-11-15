-- ==============================================================================
-- HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC - PBL2 (DE TAI 24)
-- File: 04_seed_khoa_nganh.sql
-- Muc tieu: Nap Danh muc Khoa, Danh muc Nganh, Khoa hoc va Chi tieu tuyen sinh K26
-- Quy chuan: GEMINI.md - 100% lowerCamelCase
-- ==============================================================================

USE `pbl2_tuyensinh`;

-- 1. SEED BANG KHOA DAO TAO DUT (6 Khoa ky thuat tieu bieu)
INSERT INTO `khoa` (`maKhoa`, `tenKhoa`, `maKhoaTheoChuanDUT`) VALUES
('CNTT', 'Khoa Cong nghe Thong tin', '102'),
('DIEN', 'Khoa Dien', '105'),
('DTVT', 'Khoa Dien tu - Vien thong', '106'),
('CKHI', 'Khoa Co khi', '101'),
('HOA',  'Khoa Hoa', '107'),
('XDDD', 'Khoa Xay dung Dan dung & Cong nghiep', '109')
ON DUPLICATE KEY UPDATE `tenKhoa` = VALUES(`tenKhoa`), `maKhoaTheoChuanDUT` = VALUES(`maKhoaTheoChuanDUT`);

-- 2. SEED BANG DANH MUC NGANH DUT (12 Nganh / Chuyen nganh chuan Bo GD&DT va DUT)
INSERT INTO `danh_muc_nganh` (`maNganh`, `tenNganh`, `maKhoa`, `tienToLop`) VALUES
('7480201B', 'CNTT (Khoa hoc Du lieu & Tri tue Nhan tao)', 'CNTT', 'TCLC_KHDL'),
('7480201',  'Cong nghe Thong tin (Chuong trinh Dac thu)',    'CNTT', 'T_DT'),
('7480201A', 'Cong nghe Thong tin (Dinh huong Ngoai ngu Nhat)', 'CNTT', 'T_N'),
('7480106',  'Ky thuat May tinh',                              'CNTT', 'KTMT'),
('7520216',  'Ky thuat Dieu khien & Tu dong hoa',             'DIEN', 'TDH'),
('7520201',  'Ky thuat Dien (He thong dien)',                  'DIEN', 'D'),
('7520207A', 'Vi dien tu - Thiet ke vi mach (Ban dan)',       'DTVT', 'VM'),
('7520207',  'Ky thuat Dien tu - Vien thong',                  'DTVT', 'DT'),
('7520114',  'Ky thuat Co dien tu',                            'CKHI', 'CDT'),
('7520130',  'Ky thuat O to',                                  'CKHI', 'OTO'),
('7520301',  'Ky thuat Hoa hoc',                               'HOA',  'H'),
('7580201',  'Ky thuat Xay dung',                              'XDDD', 'X')
ON DUPLICATE KEY UPDATE `tenNganh` = VALUES(`tenNganh`), `maKhoa` = VALUES(`maKhoa`), `tienToLop` = VALUES(`tienToLop`);

-- 3. SEED BANG KHOA HOC (Khoa 2026 - K26)
INSERT INTO `khoa_hoc` (`namTuyenSinh`, `tenKhoaHoc`, `trangThai`) VALUES
(2026, 'Khoa 2026 (K26 - Dai hoc Chinh quy)', 'DANG TUYEN SINH'),
(2025, 'Khoa 2025 (K25 - Dai hoc Chinh quy)', 'DA KET THUC')
ON DUPLICATE KEY UPDATE `tenKhoaHoc` = VALUES(`tenKhoaHoc`), `trangThai` = VALUES(`trangThai`);

-- 4. SEED BANG NGANH THEO KHOA HOC (Chi tieu va Diem san K26 - Cho phep sua dong tren UI)
INSERT INTO `nganh_theo_khoa_hoc` (`namTuyenSinh`, `maNganh`, `chiTieu`, `diemChuanChot`, `diemSanToiThieu`) VALUES
(2026, '7480201B', 60,  0.0, 15.00),
(2026, '7480201',  120, 0.0, 15.00),
(2026, '7480201A', 60,  0.0, 15.00),
(2026, '7480106',  60,  0.0, 15.00),
(2026, '7520216',  90,  0.0, 15.00),
(2026, '7520201',  90,  0.0, 15.00),
(2026, '7520207A', 60,  0.0, 15.00),
(2026, '7520207',  90,  0.0, 15.00),
(2026, '7520114',  90,  0.0, 15.00),
(2026, '7520130',  90,  0.0, 15.00),
(2026, '7520301',  60,  0.0, 15.00),
(2026, '7580201',  60,  0.0, 15.00)
ON DUPLICATE KEY UPDATE `chiTieu` = VALUES(`chiTieu`), `diemSanToiThieu` = VALUES(`diemSanToiThieu`);
