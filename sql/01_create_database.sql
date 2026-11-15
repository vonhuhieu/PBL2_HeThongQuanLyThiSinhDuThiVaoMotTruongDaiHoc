-- ==============================================================================
-- HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC - PBL2 (DE TAI 24)
-- KHOA CONG NGHE THONG TIN - TRUONG DAI HOC BACH KHOA, DH DA NANG
-- File: 01_create_database.sql
-- Muc tieu: Khoi tao CSDL pbl2_tuyensinh va he thong 10 bang chuan 3NF (lowerCamelCase)
-- Quy chuan: GEMINI.md - Dong nhat 100% giua Model C++ va Bang MySQL
-- ==============================================================================

CREATE DATABASE IF NOT EXISTS `pbl2_tuyensinh`
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

USE `pbl2_tuyensinh`;

-- Xoa bang theo thu tu rang buoc khoa ngoai nguoc lai
DROP TABLE IF EXISTS `nguyen_vong`;
DROP TABLE IF EXISTS `thisinh`;
DROP TABLE IF EXISTS `nguoi`;
DROP TABLE IF EXISTS `nganh_theo_khoa_hoc`;
DROP TABLE IF EXISTS `danh_muc_nganh`;
DROP TABLE IF EXISTS `khoa`;
DROP TABLE IF EXISTS `khoa_hoc`;
DROP TABLE IF EXISTS `tinh_thanh`;
DROP TABLE IF EXISTS `gioi_tinh`;
DROP TABLE IF EXISTS `cau_hinh`;

-- 1. BANG DANH MUC TINH / THANH PHO
CREATE TABLE `tinh_thanh` (
    `maTinh`   VARCHAR(10)     NOT NULL,
    `tenTinh`  VARCHAR(100)    NOT NULL,
    `loai`     VARCHAR(50)     DEFAULT 'Tinh',
    PRIMARY KEY (`maTinh`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 2. BANG DANH MUC GIOI TINH
CREATE TABLE `gioi_tinh` (
    `maGioiTinh`   VARCHAR(10)    NOT NULL,
    `tenGioiTinh`  VARCHAR(30)    NOT NULL,
    PRIMARY KEY (`maGioiTinh`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 3. BANG KHOA HOC / NAM TUYEN SINH
CREATE TABLE `khoa_hoc` (
    `namTuyenSinh` INT            NOT NULL,
    `tenKhoaHoc`   VARCHAR(100)   NOT NULL,
    `trangThai`    VARCHAR(30)    DEFAULT 'DANG TUYEN SINH',
    PRIMARY KEY (`namTuyenSinh`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 4. BANG KHOA DAO TAO
CREATE TABLE `khoa` (
    `maKhoa`              VARCHAR(10)     NOT NULL,
    `tenKhoa`             VARCHAR(100)    NOT NULL,
    `maKhoaTheoChuanDUT`  VARCHAR(10)     NOT NULL,
    PRIMARY KEY (`maKhoa`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 5. BANG DANH MUC NGANH (Tinh)
CREATE TABLE `danh_muc_nganh` (
    `maNganh`    VARCHAR(20)     NOT NULL,
    `tenNganh`   VARCHAR(150)    NOT NULL,
    `maKhoa`     VARCHAR(10)     NOT NULL,
    `tienToLop`  VARCHAR(30)     NOT NULL,
    PRIMARY KEY (`maNganh`),
    CONSTRAINT `fk_nganh_khoa`
        FOREIGN KEY (`maKhoa`) REFERENCES `khoa` (`maKhoa`)
        ON UPDATE CASCADE ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 6. BANG NGANH THEO KHOA HOC (Bien dong chi tieu va diem chuan theo nam)
CREATE TABLE `nganh_theo_khoa_hoc` (
    `namTuyenSinh`    INT             NOT NULL,
    `maNganh`         VARCHAR(20)     NOT NULL,
    `chiTieu`         INT             NOT NULL DEFAULT 60,
    `diemChuanChot`   DOUBLE          DEFAULT 0.0,
    `diemSanToiThieu` DOUBLE          DEFAULT 15.00,
    PRIMARY KEY (`namTuyenSinh`, `maNganh`),
    CONSTRAINT `fk_ntkh_khoa_hoc`
        FOREIGN KEY (`namTuyenSinh`) REFERENCES `khoa_hoc` (`namTuyenSinh`)
        ON UPDATE CASCADE ON DELETE CASCADE,
    CONSTRAINT `fk_ntkh_nganh`
        FOREIGN KEY (`maNganh`) REFERENCES `danh_muc_nganh` (`maNganh`)
        ON UPDATE CASCADE ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 7. BANG LOP CO SO NGUOI (Chua thong tin nhan than)
CREATE TABLE `nguoi` (
    `soCCCD`    VARCHAR(20)    NOT NULL,
    `hoTen`     VARCHAR(100)   NOT NULL,
    `ngaySinh`  VARCHAR(20)    NOT NULL,
    `queQuan`   VARCHAR(10)    NOT NULL,
    `gioiTinh`  VARCHAR(10)    NOT NULL,
    PRIMARY KEY (`soCCCD`),
    CONSTRAINT `fk_nguoi_queQuan` 
        FOREIGN KEY (`queQuan`) REFERENCES `tinh_thanh` (`maTinh`) 
        ON UPDATE CASCADE ON DELETE RESTRICT,
    CONSTRAINT `fk_nguoi_gioiTinh` 
        FOREIGN KEY (`gioiTinh`) REFERENCES `gioi_tinh` (`maGioiTinh`) 
        ON UPDATE CASCADE ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 8. BANG LOP KE THUA THI SINH
CREATE TABLE `thisinh` (
    `soBaoDanh`            VARCHAR(20)    NOT NULL,
    `soCCCD`               VARCHAR(20)    NOT NULL UNIQUE,
    `namTuyenSinh`         INT            NOT NULL DEFAULT 2026,
    `diemToan`             DOUBLE         NOT NULL,
    `diemLy`               DOUBLE         NOT NULL,
    `diemHoa`              DOUBLE         NOT NULL,
    `maSinhVien`           VARCHAR(20)    DEFAULT NULL,
    `lopSinhHoat`          VARCHAR(50)    DEFAULT NULL,
    `nganhTrungTuyen`      VARCHAR(20)    DEFAULT NULL,
    `nguyenVongTrungTuyen` INT            DEFAULT 0,
    PRIMARY KEY (`soBaoDanh`),
    CONSTRAINT `fk_thisinh_nguoi` 
        FOREIGN KEY (`soCCCD`) REFERENCES `nguoi` (`soCCCD`) 
        ON UPDATE CASCADE ON DELETE CASCADE,
    CONSTRAINT `fk_thisinh_khoa_hoc`
        FOREIGN KEY (`namTuyenSinh`) REFERENCES `khoa_hoc` (`namTuyenSinh`)
        ON UPDATE CASCADE ON DELETE RESTRICT,
    CONSTRAINT `fk_thisinh_nganh`
        FOREIGN KEY (`nganhTrungTuyen`) REFERENCES `danh_muc_nganh` (`maNganh`)
        ON UPDATE CASCADE ON DELETE SET NULL,
    CONSTRAINT `chk_diemToan` CHECK (`diemToan` >= 0.0 AND `diemToan` <= 10.0),
    CONSTRAINT `chk_diemLy`   CHECK (`diemLy`   >= 0.0 AND `diemLy`   <= 10.0),
    CONSTRAINT `chk_diemHoa`  CHECK (`diemHoa`  >= 0.0 AND `diemHoa`  <= 10.0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 9. BANG NGUYEN VONG XET TUYEN (Moi thi sinh toi da 3 NV)
CREATE TABLE `nguyen_vong` (
    `id`         INT AUTO_INCREMENT NOT NULL,
    `soBaoDanh`  VARCHAR(20)        NOT NULL,
    `thuTuNV`    INT                NOT NULL,
    `maNganh`    VARCHAR(20)        NOT NULL,
    `trangThai`  VARCHAR(30)        NOT NULL DEFAULT 'DANG XET',
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_thisinh_thuTuNV` (`soBaoDanh`, `thuTuNV`),
    CONSTRAINT `fk_nv_thisinh`
        FOREIGN KEY (`soBaoDanh`) REFERENCES `thisinh` (`soBaoDanh`)
        ON UPDATE CASCADE ON DELETE CASCADE,
    CONSTRAINT `fk_nv_nganh`
        FOREIGN KEY (`maNganh`) REFERENCES `danh_muc_nganh` (`maNganh`)
        ON UPDATE CASCADE ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 10. BANG CAU HINH HE THONG
CREATE TABLE `cau_hinh` (
    `khoa`     VARCHAR(50)     NOT NULL,
    `giaTri`   VARCHAR(255)    NOT NULL,
    `moTa`     VARCHAR(255)    DEFAULT NULL,
    PRIMARY KEY (`khoa`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
