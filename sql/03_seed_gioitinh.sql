-- ==============================================================================
-- HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC - PBL2 (DE TAI 24)
-- File: 03_seed_gioitinh.sql
-- Muc tieu: Nap danh muc Gioi tinh chuan hoa
-- ==============================================================================

USE `pbl2_tuyensinh`;

INSERT INTO `gioi_tinh` (`ma_gioi_tinh`, `ten_gioi_tinh`) VALUES
('NAM', 'Nam'),
('NU', 'Nu'),
('KHAC', 'Khac')
ON DUPLICATE KEY UPDATE `ten_gioi_tinh` = VALUES(`ten_gioi_tinh`);
