# PowerShell Generator for 100 Realistic DUT Admission Candidates
# Target major: 7480201A - Cong nghe Thong tin (Dinh huong Ngoai ngu Nhat)

$hoList = @("Nguyen", "Tran", "Le", "Pham", "Hoang", "Phan", "Vu", "Vo", "Dang", "Bui", "Do", "Ho", "Ngo", "Duong", "Ly", "Dinh", "Doan", "Lam", "Trinh", "Mai", "Cao", "Ha", "Luu", "Ta", "Truong")
$demNamList = @("Van", "Huu", "Minh", "Quoc", "Duc", "Dinh", "Thanh", "Quang", "Trong", "Hoang", "Cong", "Gia", "Bao", "Tuan", "Ngoc", "Xuan", "Tan", "Vinh", "Duy", "Phuoc")
$tenNamList = @("An", "Binh", "Cuong", "Dung", "Dat", "Hai", "Hieu", "Hoang", "Huy", "Hung", "Khang", "Khanh", "Khoa", "Kien", "Lam", "Long", "Minh", "Nam", "Nghia", "Nhan", "Nhat", "Phong", "Phuc", "Quan", "Quang", "Sang", "Son", "Tai", "Tam", "Thang", "Thinh", "Thong", "Thuan", "Tien", "Toan", "Tri", "Trung", "Tu", "Tuan", "Tung", "Viet", "Vinh", "Vu")

$demNuList = @("Thi", "Ngoc", "Thu", "Thao", "Mai", "Phuong", "Kim", "Thanh", "Quynh", "Bao", "My", "Anh", "Diem", "Cam", "Hong", "Thuy", "Khanh", "Ha", "Huyen", "Yen")
$tenNuList = @("Anh", "Anh", "Chi", "Diep", "Dung", "Duyen", "Giang", "Ha", "Han", "Hang", "Hanh", "Hien", "Hoa", "Hoai", "Huong", "Huyen", "Lan", "Linh", "Ly", "Mai", "My", "Nga", "Ngan", "Ngoc", "Nhung", "Nhu", "Oanh", "Phuong", "Quyen", "Quynh", "Tam", "Thao", "Thi", "Thu", "Thuong", "Thuy", "Thuy", "Tien", "Trang", "Tram", "Truc", "Tuyet", "Uyen", "Van", "Vi", "Vy", "Yen")

$provinces = @("48", "46", "51", "45", "42", "40", "38", "01", "79", "56", "64", "66", "31", "04", "19", "27")

$candidates = [System.Collections.Generic.List[PSObject]]::new()

# SBD 001 is Vo Nhu Hieu
$candidates.Add([PSCustomObject]@{
    sbd = "DUT001"; cccd = "048003001234"; hoTen = "Vo Nhu Hieu"; ngaySinh = "04/03/2003"; queQuan = "48"; gioiTinh = "NAM";
    toan = 9.40; ly = 9.20; hoa = 9.50; nv1 = "7480201A"; nv2 = "7480201B"; nv3 = "7480201"
})

# SBD 002 is Nguyen Hoang Minh Quan
$candidates.Add([PSCustomObject]@{
    sbd = "DUT002"; cccd = "048208001567"; hoTen = "Nguyen Hoang Minh Quan"; ngaySinh = "15/01/2008"; queQuan = "48"; gioiTinh = "NAM";
    toan = 9.80; ly = 9.60; hoa = 9.80; nv1 = "7480201A"; nv2 = "7520207A"; nv3 = "7480201"
})

# SBD 003 is Tran Le Thao Vy
$candidates.Add([PSCustomObject]@{
    sbd = "DUT003"; cccd = "048308002341"; hoTen = "Tran Le Thao Vy"; ngaySinh = "22/04/2008"; queQuan = "48"; gioiTinh = "NU";
    toan = 9.60; ly = 9.40; hoa = 9.75; nv1 = "7480201A"; nv2 = "7480201B"; nv3 = "7480201"
})

# Generate scores descending from 28.50 down to ~14.00 for candidates 4 to 100
$rnd = [System.Random]::new(2026)

for ($i = 4; $i -le 100; $i++) {
    $sbdStr = "DUT" + $i.ToString("D3")
    $isMale = ($rnd.Next(100) -lt 65)
    
    $ho = $hoList[$rnd.Next($hoList.Count)]
    if ($isMale) {
        $dem = $demNamList[$rnd.Next($demNamList.Count)]
        $ten = $tenNamList[$rnd.Next($tenNamList.Count)]
        $gt = "NAM"
        $genderDigit = "2" # 21st century male
    } else {
        $dem = $demNuList[$rnd.Next($demNuList.Count)]
        $ten = $tenNuList[$rnd.Next($tenNuList.Count)]
        $gt = "NU"
        $genderDigit = "3" # 21st century female
    }
    $fullName = "$ho $dem $ten"
    
    $que = $provinces[$rnd.Next($provinces.Count)]
    $day = $rnd.Next(1, 29).ToString("D2")
    $month = $rnd.Next(1, 13).ToString("D2")
    $dob = "$day/$month/2008"
    
    $randNum = $rnd.Next(100000, 999999).ToString()
    $cccd = $que.PadLeft(3, '0') + $genderDigit + "08" + $randNum

    # Score progression from high to low
    # i=4..15: ~28.00 down to 26.50
    # i=16..45: ~26.40 down to 24.00
    # i=46..75: ~23.90 down to 21.00
    # i=76..92: ~20.90 down to 16.50
    # i=93..98: ~16.40 down to 14.50
    # i=99: below floor (< 15.00)
    # i=100: diem liet
    if ($i -le 15) {
        $targetTotal = 28.0 - (($i - 4) * 0.12)
    } elseif ($i -le 45) {
        $targetTotal = 26.5 - (($i - 16) * 0.08)
    } elseif ($i -le 75) {
        $targetTotal = 24.0 - (($i - 46) * 0.10)
    } elseif ($i -le 92) {
        $targetTotal = 21.0 - (($i - 76) * 0.25)
    } elseif ($i -le 98) {
        $targetTotal = 16.5 - (($i - 93) * 0.35)
    } elseif ($i -eq 99) {
        $targetTotal = 13.80 # Under floor 15.00
    } else {
        # i=100: Math=9.00, Phys=8.50, Chem=1.00 (diem liet)
        $candidates.Add([PSCustomObject]@{
            sbd = $sbdStr; cccd = $cccd; hoTen = $fullName; ngaySinh = $dob; queQuan = $que; gioiTinh = $gt;
            toan = 9.00; ly = 8.50; hoa = 1.00; nv1 = "7480201A"; nv2 = "7480201"; nv3 = "7480106"
        })
        continue
    }

    # Split targetTotal into 3 subjects (multiples of 0.20 or 0.25)
    $base = [Math]::Round($targetTotal / 3.0, 1)
    $toan = [Math]::Min(10.0, [Math]::Max(2.0, [Math]::Round($base + (($rnd.Next(5) - 2) * 0.20), 2)))
    $ly   = [Math]::Min(10.0, [Math]::Max(2.0, [Math]::Round($base + (($rnd.Next(5) - 2) * 0.20), 2)))
    $hoa  = [Math]::Round($targetTotal - $toan - $ly, 2)
    if ($hoa -gt 10.0) {
        $diff = $hoa - 10.0
        $hoa = 10.0
        $toan = [Math]::Min(10.0, $toan + $diff)
    }
    if ($hoa -lt 1.5) { $hoa = 2.0 }

    # Aspirations distribution: all involve 7480201A
    # 70% have NV1 = 7480201A
    # 20% have NV2 = 7480201A (NV1 is 7480201B or 7480201)
    # 10% have NV3 = 7480201A (NV1 is 7480201B, NV2 is 7480201)
    $aspChoice = $rnd.Next(100)
    if ($aspChoice -lt 70) {
        $nv1 = "7480201A"
        $nv2 = "7480201"
        $nv3 = "7480106"
    } elseif ($aspChoice -lt 90) {
        $nv1 = "7480201B"
        $nv2 = "7480201A"
        $nv3 = "7480201"
    } else {
        $nv1 = "7480201B"
        $nv2 = "7480201"
        $nv3 = "7480201A"
    }

    $candidates.Add([PSCustomObject]@{
        sbd = $sbdStr; cccd = $cccd; hoTen = $fullName; ngaySinh = $dob; queQuan = $que; gioiTinh = $gt;
        toan = $toan; ly = $ly; hoa = $hoa; nv1 = $nv1; nv2 = $nv2; nv3 = $nv3
    })
}

# Generate SQL Output
$sb = [System.Text.StringBuilder]::new()
$sb.AppendLine("-- ==============================================================================") | Out-Null
$sb.AppendLine("-- HE THONG QUAN LY THI SINH TUYEN SINH DAI HOC - PBL2 (DE TAI 24)") | Out-Null
$sb.AppendLine("-- File: 05_seed_sample_data.sql") | Out-Null
$sb.AppendLine("-- Bo du lieu 100 Thi sinh xet tuyen nganh 7480201A (CNTT - Ngoai ngu Nhat)") | Out-Null
$sb.AppendLine("-- Quy chuan: GEMINI.md - 100% lowerCamelCase") | Out-Null
$sb.AppendLine("-- ==============================================================================") | Out-Null
$sb.AppendLine("") | Out-Null
$sb.AppendLine("USE ``pbl2_tuyensinh``;") | Out-Null
$sb.AppendLine("") | Out-Null
$sb.AppendLine("-- 1. BANG CAU HINH HE THONG (Global Settings)") | Out-Null
$sb.AppendLine("INSERT INTO ``cau_hinh`` (``khoa``, ``giaTri``, ``moTa``) VALUES") | Out-Null
$sb.AppendLine("('tenTruong',             'TRUONG DAI HOC BACH KHOA - DAI HOC DA NANG', 'Ten truong tuyen sinh chinh thuc'),") | Out-Null
$sb.AppendLine("('namTuyenSinhHienHanh',  '2026',                                      'Khoa tuyen sinh dai hoc mac dinh'),") | Out-Null
$sb.AppendLine("('siSoLopChuan',          '30',                                        'Si so tieu chuan dong cua 1 lop sinh hoat DUT'),") | Out-Null
$sb.AppendLine("('diemLiet',              '1.00',                                      'Nguong diem liet mon thi theo quy che'),") | Out-Null
$sb.AppendLine("('diemSanMacDinh',        '15.00',                                     'Diem san toi thieu nhan ho so xet tuyen')") | Out-Null
$sb.AppendLine("ON DUPLICATE KEY UPDATE ``giaTri`` = VALUES(``giaTri``), ``moTa`` = VALUES(``moTa``);") | Out-Null
$sb.AppendLine("") | Out-Null

$sb.AppendLine("-- 2. DU LIEU BANG NGUOI (100 Thi sinh)") | Out-Null
$sb.AppendLine("INSERT INTO ``nguoi`` (``soCCCD``, ``hoTen``, ``ngaySinh``, ``queQuan``, ``gioiTinh``) VALUES") | Out-Null
for ($i = 0; $i -lt $candidates.Count; $i++) {
    $c = $candidates[$i]
    $comma = if ($i -lt $candidates.Count - 1) { "," } else { "" }
    $sb.AppendLine("('$($c.cccd)', '$($c.hoTen)', '$($c.ngaySinh)', '$($c.queQuan)', '$($c.gioiTinh)')$comma") | Out-Null
}
$sb.AppendLine("ON DUPLICATE KEY UPDATE ``hoTen`` = VALUES(``hoTen``), ``ngaySinh`` = VALUES(``ngaySinh``), ``queQuan`` = VALUES(``queQuan``), ``gioiTinh`` = VALUES(``gioiTinh``);") | Out-Null
$sb.AppendLine("") | Out-Null

$sb.AppendLine("-- 3. DU LIEU BANG THISINH (100 Thi sinh)") | Out-Null
$sb.AppendLine("INSERT INTO ``thisinh`` (``soBaoDanh``, ``soCCCD``, ``namTuyenSinh``, ``diemToan``, ``diemLy``, ``diemHoa``, ``maSinhVien``, ``lopSinhHoat``, ``nganhTrungTuyen``, ``nguyenVongTrungTuyen``) VALUES") | Out-Null
for ($i = 0; $i -lt $candidates.Count; $i++) {
    $c = $candidates[$i]
    $comma = if ($i -lt $candidates.Count - 1) { "," } else { "" }
    $toanStr = [string]::Format([System.Globalization.CultureInfo]::InvariantCulture, "{0:0.00}", $c.toan)
    $lyStr   = [string]::Format([System.Globalization.CultureInfo]::InvariantCulture, "{0:0.00}", $c.ly)
    $hoaStr  = [string]::Format([System.Globalization.CultureInfo]::InvariantCulture, "{0:0.00}", $c.hoa)
    $sb.AppendLine("('$($c.sbd)', '$($c.cccd)', 2026, $toanStr, $lyStr, $hoaStr, NULL, NULL, NULL, 0)$comma") | Out-Null
}
$sb.AppendLine("ON DUPLICATE KEY UPDATE ``diemToan`` = VALUES(``diemToan``), ``diemLy`` = VALUES(``diemLy``), ``diemHoa`` = VALUES(``diemHoa``);") | Out-Null
$sb.AppendLine("") | Out-Null

$sb.AppendLine("-- 4. DU LIEU BANG NGUYEN VONG (Dang ky toi da 3 NV moi thi sinh)") | Out-Null
$sb.AppendLine("INSERT INTO ``nguyen_vong`` (``soBaoDanh``, ``thuTuNV``, ``maNganh``, ``trangThai``) VALUES") | Out-Null
$nvList = [System.Collections.Generic.List[string]]::new()
for ($i = 0; $i -lt $candidates.Count; $i++) {
    $c = $candidates[$i]
    $nvList.Add("('$($c.sbd)', 1, '$($c.nv1)', 'DANG XET')")
    if ($c.nv2) { $nvList.Add("('$($c.sbd)', 2, '$($c.nv2)', 'DANG XET')") }
    if ($c.nv3) { $nvList.Add("('$($c.sbd)', 3, '$($c.nv3)', 'DANG XET')") }
}
for ($k = 0; $k -lt $nvList.Count; $k++) {
    $comma = if ($k -lt $nvList.Count - 1) { "," } else { "" }
    $sb.AppendLine("$($nvList[$k])$comma") | Out-Null
}
$sb.AppendLine("ON DUPLICATE KEY UPDATE ``maNganh`` = VALUES(``maNganh``), ``trangThai`` = VALUES(``trangThai``);") | Out-Null

[System.IO.File]::WriteAllText("d:\HOCTAP\pbl2\pbl2_MySQL\sql\05_seed_sample_data.sql", $sb.ToString(), [System.Text.Encoding]::UTF8)
Write-Output "Successfully generated 100 realistic candidates into sql/05_seed_sample_data.sql"
