// fileio.h
#ifndef FILEIO_H
#define FILEIO_H

#include "structs.h"
#include "monhoc_avl.h"
#include "examlog.h"

// ========================= FORMAT FILE ===========================
//
// 1) Lop.txt
//    Dùng để lưu danh sách LỚP (không lưu sinh viên)
//    ----------------------------------------------------------------
//    SoLop
//    MALOP_1, TENLOP_1, SoSV_1
//    MALOP_2, TENLOP_2, SoSV_2
//    ...
//
//    Ghi chú:
//    - SoSV_* là số SV hiện có trong lớp, chỉ dùng để tham khảo/kiểm tra,
//      khi đọc file có thể bỏ qua (không dùng để build DS).
//
// 2) SinhVien.txt
//    Dùng để lưu toàn bộ sinh viên của tất cả lớp
//    ----------------------------------------------------------------
//    SoSV
//    MALOP, MASV, HO, TEN, PHAI, PASSWORD
//    ...
//
// 3) MonHoc.txt
//    ----------------------------------------------------------------
//    SoMon
//    MAMH, TENMH
//    ...
//
// 4) CauHoi.txt
//    ----------------------------------------------------------------
//    TongCau
//    id, mamh, noidung, A, B, C, D, dapan
//    ...
//
// 5) SinhVienDaThi.txt
//    // giữ nguyên format cũ (nhiều dòng/record)
// =================================================================

// Lớp
bool doc_lop_txt(const char* path, DS_Lop& ds);
bool ghi_lop_txt(const char* path, const DS_Lop& ds);

// Sinh viên
bool doc_sinhvien_txt(const char* path, DS_Lop& ds);
bool ghi_sinhvien_txt(const char* path, const DS_Lop& ds);

// Môn học
bool doc_monhoc_txt(const char* path, PTRMH& root);
bool ghi_monhoc_txt(const char* path, PTRMH root);

// Câu hỏi
bool doc_cauhoi_txt(const char* path, PTRMH root);
bool ghi_cauhoi_txt(const char* path, PTRMH root);

// Log sinh viên đã thi
bool doc_sinhvien_dathi_txt(const char* path, PTRExamLog& logs, DS_Lop& ds);
bool ghi_sinhvien_dathi_txt(const char* path, PTRExamLog logs);

#endif // FILEIO_H
