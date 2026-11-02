-- ==============================================================================
-- HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC - PBL2 (DE TAI 24)
-- KHOA CONG NGHE THONG TIN - TRUONG DAI HOC BACH KHOA, DH DA NANG
-- File: 01_create_database.sql
-- Muc tieu: Khoi tao CSDL pbl2_tuyensinh va he thong 5 bang chuan hoa 3NF
-- ==============================================================================

CREATE DATABASE IF NOT EXISTS `pbl2_tuyensinh`
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

USE `pbl2_tuyensinh`;

-- 1. BANG DANH MUC TINH / THANH PHO (Tra cuu Que quan)
DROP TABLE IF EXISTS `thisinh`;
DROP TABLE IF EXISTS `nguoi`;
DROP TABLE IF EXISTS `tinh_thanh`;
DROP TABLE IF EXISTS `gioi_tinh`;
DROP TABLE IF EXISTS `cau_hinh`;

CREATE TABLE `tinh_thanh` (
    `ma_tinh`   VARCHAR(10)     NOT NULL,
    `ten_tinh`  VARCHAR(100)    NOT NULL,
    `loai`      VARCHAR(50)     DEFAULT 'Tinh',
    PRIMARY KEY (`ma_tinh`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 2. BANG DANH MUC GIOI TINH (Chuan hoa tu nhien: NAM, NU, KHAC)
CREATE TABLE `gioi_tinh` (
    `ma_gioi_tinh`   VARCHAR(10)    NOT NULL,
    `ten_gioi_tinh`  VARCHAR(30)    NOT NULL,
    PRIMARY KEY (`ma_gioi_tinh`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 3. BANG LOP CO SO NGUOI (Chua thong tin ca nhan cong dan)
CREATE TABLE `nguoi` (
    `so_cccd`    VARCHAR(20)    NOT NULL,
    `ho_ten`     VARCHAR(100)   NOT NULL,
    `ngay_sinh`  VARCHAR(20)    NOT NULL,
    `que_quan`   VARCHAR(10)    NOT NULL,
    `gioi_tinh`  VARCHAR(10)    NOT NULL,
    PRIMARY KEY (`so_cccd`),
    CONSTRAINT `fk_nguoi_que_quan` 
        FOREIGN KEY (`que_quan`) REFERENCES `tinh_thanh` (`ma_tinh`) 
        ON UPDATE CASCADE ON DELETE RESTRICT,
    CONSTRAINT `fk_nguoi_gioi_tinh` 
        FOREIGN KEY (`gioi_tinh`) REFERENCES `gioi_tinh` (`ma_gioi_tinh`) 
        ON UPDATE CASCADE ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 4. BANG LOP KE THUA THI SINH (Chua thong tin du thi va diem so)
-- Mo hinh Class Table Inheritance (1-1 voi bang nguoi qua so_cccd)
CREATE TABLE `thisinh` (
    `sbd`        VARCHAR(20)    NOT NULL,
    `so_cccd`    VARCHAR(20)    NOT NULL UNIQUE,
    `diem_toan`  DOUBLE         NOT NULL,
    `diem_ly`    DOUBLE         NOT NULL,
    `diem_hoa`   DOUBLE         NOT NULL,
    PRIMARY KEY (`sbd`),
    CONSTRAINT `fk_thisinh_nguoi` 
        FOREIGN KEY (`so_cccd`) REFERENCES `nguoi` (`so_cccd`) 
        ON UPDATE CASCADE ON DELETE CASCADE,
    CONSTRAINT `chk_diem_toan` CHECK (`diem_toan` >= 0.0 AND `diem_toan` <= 10.0),
    CONSTRAINT `chk_diem_ly`   CHECK (`diem_ly`   >= 0.0 AND `diem_ly`   <= 10.0),
    CONSTRAINT `chk_diem_hoa`  CHECK (`diem_hoa`  >= 0.0 AND `diem_hoa`  <= 10.0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 5. BANG CAU HINH HE THONG
CREATE TABLE `cau_hinh` (
    `khoa`     VARCHAR(50)     NOT NULL,
    `gia_tri`  VARCHAR(255)    NOT NULL,
    `mo_ta`    VARCHAR(255)    DEFAULT NULL,
    PRIMARY KEY (`khoa`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

