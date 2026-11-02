-- ==============================================================================
-- HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC - PBL2 (DE TAI 24)
-- File: 02_seed_tinhthanh.sql
-- Muc tieu: Nap danh muc 34 Tinh / Thanh pho
-- ==============================================================================

USE `pbl2_tuyensinh`;

INSERT INTO `tinh_thanh` (`ma_tinh`, `ten_tinh`, `loai`) VALUES
('01', 'TP. Ha Noi', 'Thanh pho truc thuoc Trung uong'),
('04', 'Cao Bang', 'Tinh'),
('08', 'Tuyen Quang', 'Tinh'),
('10', 'Lao Cai', 'Tinh'),
('11', 'Dien Bien', 'Tinh'),
('12', 'Lai Chau', 'Tinh'),
('14', 'Son La', 'Tinh'),
('19', 'Thai Nguyen', 'Tinh'),
('20', 'Lang Son', 'Tinh'),
('22', 'Quang Ninh', 'Tinh'),
('25', 'Phu Tho', 'Tinh'),
('27', 'Bac Ninh', 'Tinh'),
('31', 'TP. Hai Phong', 'Thanh pho truc thuoc Trung uong'),
('33', 'Hung Yen', 'Tinh'),
('37', 'Ninh Binh', 'Tinh'),
('38', 'Thanh Hoa', 'Tinh'),
('40', 'Nghe An', 'Tinh'),
('42', 'Ha Tinh', 'Tinh'),
('45', 'Quang Tri', 'Tinh'),
('46', 'TP. Hue', 'Thanh pho truc thuoc Trung uong'),
('48', 'TP. Da Nang', 'Thanh pho truc thuoc Trung uong'),
('51', 'Quang Ngai', 'Tinh'),
('56', 'Khanh Hoa', 'Tinh'),
('64', 'Gia Lai', 'Tinh'),
('66', 'Dak Lak', 'Tinh'),
('68', 'Lam Dong', 'Tinh'),
('72', 'Tay Ninh', 'Tinh'),
('75', 'Dong Nai', 'Tinh'),
('79', 'TP. Ho Chi Minh', 'Thanh pho truc thuoc Trung uong'),
('86', 'Vinh Long', 'Tinh'),
('87', 'Dong Thap', 'Tinh'),
('89', 'An Giang', 'Tinh'),
('92', 'TP. Can Tho', 'Thanh pho truc thuoc Trung uong')
ON DUPLICATE KEY UPDATE `ten_tinh` = VALUES(`ten_tinh`), `loai` = VALUES(`loai`);
