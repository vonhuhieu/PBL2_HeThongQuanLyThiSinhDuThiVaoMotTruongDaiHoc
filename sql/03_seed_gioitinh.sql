-- ==============================================================================
-- HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC - PBL2 (DE TAI 24)
-- File: 03_seed_gioitinh.sql
-- Muc tieu: Nap danh muc Gioi tinh chuan hoa (lowerCamelCase: maGioiTinh, tenGioiTinh)
-- ==============================================================================

USE `pbl2_tuyensinh`;

INSERT INTO `gioi_tinh` (`maGioiTinh`, `tenGioiTinh`) VALUES
('NAM', 'Nam'),
('NU', 'Nu'),
('KHAC', 'Khac')
ON DUPLICATE KEY UPDATE `tenGioiTinh` = VALUES(`tenGioiTinh`);
