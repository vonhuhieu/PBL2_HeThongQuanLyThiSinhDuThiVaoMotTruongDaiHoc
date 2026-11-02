-- ==============================================================================
-- HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC - PBL2 (DE TAI 24)
-- File: 04_seed_sample_data.sql
-- Muc tieu: Nap cau hinh he thong va du lieu thi sinh mau
-- ==============================================================================

USE `pbl2_tuyensinh`;

-- 1. CAU HINH HE THONG
INSERT INTO `cau_hinh` (`khoa`, `gia_tri`, `mo_ta`) VALUES
('truong', 'TRUONG DAI HOC BACH KHOA - DAI HOC DA NANG', 'Ten truong tuyen sinh'),
('nam_tuyen_sinh', '2026', 'Khoa tuyen sinh dai hoc'),
('diem_chuan', '23.00', 'Diem chuan trung tuyen (thang 30)'),
('diem_liet', '1.00', 'Muc diem liet theo quy che')
ON DUPLICATE KEY UPDATE `gia_tri` = VALUES(`gia_tri`), `mo_ta` = VALUES(`mo_ta`);

-- 2. DU LIEU BANG NGUOI (14 Thi sinh mau)
INSERT INTO `nguoi` (`so_cccd`, `ho_ten`, `ngay_sinh`, `que_quan`, `gioi_tinh`) VALUES
('012345678901', 'Vo Nhu Hieu', '04/03/2003', '48', 'NAM'),
('012345678902', 'Vo Huy Tuong', '27/07/2009', '48', 'NAM'),
('012345678903', 'Nguyen Thi Nhat Vy', '10/09/2005', '51', 'NU'),
('201948572134', 'Nguyen Van An', '15/03/2005', '48', 'NAM'),
('201837461928', 'Tran Thi Bich', '22/07/2005', '48', 'NU'),
('201726354819', 'Le Hoang Cong', '10/11/2005', '46', 'NAM'),
('201615243701', 'Pham Thi Diem', '05/01/2005', '51', 'NU'),
('201504132692', 'Vo Minh Duc', '18/09/2005', '42', 'NAM'),
('201493021583', 'Hoang Thi Ha', '30/04/2005', '40', 'NU'),
('201382910474', 'Dang Quoc Khanh', '12/12/2005', '45', 'NAM'),
('201271809365', 'Bui Mai Linh', '08/08/2005', '01', 'NU'),
('201160798256', 'Ngo Van Nam', '25/02/2005', '79', 'NAM'),
('201059687147', 'Dinh Thao Quyen', '19/10/2005', '56', 'NU'),
('200948576038', 'Phan Thanh Tung', '03/06/2005', '48', 'NAM')
ON DUPLICATE KEY UPDATE `ho_ten` = VALUES(`ho_ten`), `ngay_sinh` = VALUES(`ngay_sinh`), `que_quan` = VALUES(`que_quan`), `gioi_tinh` = VALUES(`gioi_tinh`);

-- 3. DU LIEU BANG THISINH (Diem thi tuong ung voi CCCD)
INSERT INTO `thisinh` (`sbd`, `so_cccd`, `diem_toan`, `diem_ly`, `diem_hoa`) VALUES
('DUT001', '012345678901', 9.00, 7.00, 8.00),
('DUT002', '012345678902', 9.00, 9.75, 8.50),
('DUT003', '012345678903', 9.50, 9.50, 8.25),
('DUT004', '201948572134', 9.50, 9.00, 9.75),
('DUT005', '201837461928', 8.75, 8.50, 9.00),
('DUT006', '201726354819', 7.50, 6.75, 7.00),
('DUT007', '201615243701', 9.00, 8.75, 8.25),
('DUT008', '201504132692', 6.25, 5.50, 6.00),
('DUT009', '201493021583', 9.75, 9.50, 9.80),
('DUT010', '201382910474', 5.00, 4.50, 5.50),
('DUT011', '201271809365', 8.50, 8.25, 8.75),
('DUT012', '201160798256', 7.00, 7.25, 6.50),
('DUT013', '201059687147', 8.00, 8.50, 7.75),
('DUT014', '200948576038', 9.25, 9.00, 9.25)
ON DUPLICATE KEY UPDATE `diem_toan` = VALUES(`diem_toan`), `diem_ly` = VALUES(`diem_ly`), `diem_hoa` = VALUES(`diem_hoa`);
