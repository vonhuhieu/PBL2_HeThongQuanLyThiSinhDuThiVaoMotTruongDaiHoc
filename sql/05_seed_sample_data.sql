-- ==============================================================================
-- HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC - PBL2 (DE TAI 24)
-- File: 05_seed_sample_data.sql
-- Muc tieu: Nap Cau hinh he thong, Thong tin thi sinh va Nguyen vong xet tuyen K26
-- Quy chuan: GEMINI.md - 100% lowerCamelCase
-- ==============================================================================

USE `pbl2_tuyensinh`;

-- 1. BANG CAU HINH HE THONG (Global Settings)
INSERT INTO `cau_hinh` (`khoa`, `giaTri`, `moTa`) VALUES
('tenTruong',             'TRUONG DAI HOC BACH KHOA - DAI HOC DA NANG', 'Ten truong tuyen sinh chinh thuc'),
('namTuyenSinhHienHanh',  '2026',                                      'Khoa tuyen sinh dai hoc mac dinh'),
('siSoLopChuan',          '30',                                        'Si so tieu chuan dong cua 1 lop sinh hoat DUT'),
('diemLiet',              '1.00',                                      'Nguong diem liet mon thi theo quy che'),
('diemSanMacDinh',        '15.00',                                     'Diem san toi thieu nhan ho so xet tuyen')
ON DUPLICATE KEY UPDATE `giaTri` = VALUES(`giaTri`), `moTa` = VALUES(`moTa`);

-- 2. DU LIEU BANG NGUOI (14 Thi sinh mau)
INSERT INTO `nguoi` (`soCCCD`, `hoTen`, `ngaySinh`, `queQuan`, `gioiTinh`) VALUES
('012345678901', 'Vo Nhu Hieu',        '04/03/2003', '48', 'NAM'),
('012345678902', 'Vo Huy Tuong',       '27/07/2009', '48', 'NAM'),
('012345678903', 'Nguyen Thi Nhat Vy',  '10/09/2005', '51', 'NU'),
('201948572134', 'Nguyen Van An',       '15/03/2005', '48', 'NAM'),
('201837461928', 'Tran Thi Bich',       '22/07/2005', '48', 'NU'),
('201726354819', 'Le Hoang Cong',       '10/11/2005', '46', 'NAM'),
('201615243701', 'Pham Thi Diem',       '05/01/2005', '51', 'NU'),
('201504132692', 'Vo Minh Duc',         '18/09/2005', '42', 'NAM'),
('201493021583', 'Hoang Thi Ha',        '30/04/2005', '40', 'NU'),
('201382910474', 'Dang Quoc Khanh',     '12/12/2005', '45', 'NAM'),
('201271809365', 'Bui Mai Linh',        '08/08/2005', '01', 'NU'),
('201160798256', 'Ngo Van Nam',         '25/02/2005', '79', 'NAM'),
('201059687147', 'Dinh Thao Quyen',     '19/10/2005', '56', 'NU'),
('200948576038', 'Phan Thanh Tung',     '03/06/2005', '48', 'NAM')
ON DUPLICATE KEY UPDATE `hoTen` = VALUES(`hoTen`), `ngaySinh` = VALUES(`ngaySinh`), `queQuan` = VALUES(`queQuan`), `gioiTinh` = VALUES(`gioiTinh`);

-- 3. DU LIEU BANG THISINH (Diem thi Khoa 2026)
INSERT INTO `thisinh` (`soBaoDanh`, `soCCCD`, `namTuyenSinh`, `diemToan`, `diemLy`, `diemHoa`, `maSinhVien`, `lopSinhHoat`, `nganhTrungTuyen`, `nguyenVongTrungTuyen`) VALUES
('DUT001', '012345678901', 2026, 9.00, 7.00, 8.00, NULL, NULL, NULL, 0),
('DUT002', '012345678902', 2026, 9.00, 9.75, 8.50, NULL, NULL, NULL, 0),
('DUT003', '012345678903', 2026, 9.50, 9.50, 8.25, NULL, NULL, NULL, 0),
('DUT004', '201948572134', 2026, 9.50, 9.00, 9.75, NULL, NULL, NULL, 0),
('DUT005', '201837461928', 2026, 8.75, 8.50, 9.00, NULL, NULL, NULL, 0),
('DUT006', '201726354819', 2026, 7.50, 6.75, 7.00, NULL, NULL, NULL, 0),
('DUT007', '201615243701', 2026, 9.00, 8.75, 8.25, NULL, NULL, NULL, 0),
('DUT008', '201504132692', 2026, 6.25, 5.50, 6.00, NULL, NULL, NULL, 0),
('DUT009', '201493021583', 2026, 9.75, 9.50, 9.80, NULL, NULL, NULL, 0),
('DUT010', '201382910474', 2026, 5.00, 4.50, 5.50, NULL, NULL, NULL, 0),
('DUT011', '201271809365', 2026, 8.50, 8.25, 8.75, NULL, NULL, NULL, 0),
('DUT012', '201160798256', 2026, 7.00, 7.25, 6.50, NULL, NULL, NULL, 0),
('DUT013', '201059687147', 2026, 8.00, 8.50, 7.75, NULL, NULL, NULL, 0),
('DUT014', '200948576038', 2026, 9.25, 9.00, 9.25, NULL, NULL, NULL, 0)
ON DUPLICATE KEY UPDATE `diemToan` = VALUES(`diemToan`), `diemLy` = VALUES(`diemLy`), `diemHoa` = VALUES(`diemHoa`);

-- 4. DU LIEU BANG NGUYEN VONG (Dang ky toi da 3 NV moi thi sinh)
INSERT INTO `nguyen_vong` (`soBaoDanh`, `thuTuNV`, `maNganh`, `trangThai`) VALUES
('DUT001', 1, '7480201B', 'DANG XET'),
('DUT001', 2, '7480201',  'DANG XET'),
('DUT001', 3, '7480106',  'DANG XET'),

('DUT002', 1, '7480201B', 'DANG XET'),
('DUT002', 2, '7520207A', 'DANG XET'),

('DUT003', 1, '7480201B', 'DANG XET'),
('DUT003', 2, '7480201',  'DANG XET'),

('DUT004', 1, '7480201B', 'DANG XET'),
('DUT004', 2, '7520216',  'DANG XET'),

('DUT005', 1, '7480201',  'DANG XET'),
('DUT005', 2, '7480201B', 'DANG XET'),

('DUT006', 1, '7520201',  'DANG XET'),
('DUT006', 2, '7520114',  'DANG XET'),

('DUT007', 1, '7480201B', 'DANG XET'),
('DUT007', 2, '7480201',  'DANG XET'),

('DUT008', 1, '7580201',  'DANG XET'),
('DUT008', 2, '7520301',  'DANG XET'),

('DUT009', 1, '7480201B', 'DANG XET'),
('DUT009', 2, '7520207A', 'DANG XET'),

('DUT010', 1, '7580201',  'DANG XET'),

('DUT011', 1, '7520216',  'DANG XET'),
('DUT011', 2, '7520207A', 'DANG XET'),

('DUT012', 1, '7520130',  'DANG XET'),
('DUT012', 2, '7520114',  'DANG XET'),

('DUT013', 1, '7520207A', 'DANG XET'),
('DUT013', 2, '7480201',  'DANG XET'),

('DUT014', 1, '7480201B', 'DANG XET'),
('DUT014', 2, '7520216',  'DANG XET')
ON DUPLICATE KEY UPDATE `maNganh` = VALUES(`maNganh`), `trangThai` = VALUES(`trangThai`);
