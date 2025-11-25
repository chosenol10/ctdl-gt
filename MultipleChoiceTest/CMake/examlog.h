// examlog.h
#ifndef EXAMLOG_H
#define EXAMLOG_H

#include "structs.h"

// Lưu chi tiết 1 câu trong bài thi
struct ExamQItem {
    int id;
    char mamh[16];
    char noidung[501];
    char A[201], B[201], C[201], D[201];
    char dapan;     // đáp án đúng
    char da_chon;   // đáp án SV chọn ('A'..'D' hoặc '?' nếu bỏ qua)
};

// Bài thi theo (masv, mamh)
struct ExamRecord {
    char masv[16];
    char mamh[16];
    int  soCau;
    ExamQItem* items; // mảng length = soCau
    int  soCauDung;
    float diem;
    ExamRecord* next;
};

typedef ExamRecord* PTRExamLog;

// Khởi tạo/giải phóng
void init_examlog(PTRExamLog& head);
void free_examlog(PTRExamLog& head);

// Tìm bản ghi (masv, mamh)
PTRExamLog find_exam(PTRExamLog head, const char* masv_ci, const char* mamh_ci);

// Thêm/sửa (upsert) một bài thi (copy dữ liệu vào DS mới)
void upsert_exam(PTRExamLog& head,
                 const char* masv_ci,
                 const char* mamh_ci,
                 const ExamQItem* items, int soCau,
                 int soCauDung, float diem);

// Kiểm tra câu hỏi (mamh,id) đã xuất hiện trong bất kỳ bài thi nào?
bool examlog_contains_question(PTRExamLog head, const char* mamh_ci, int id);

// Xoá tất cả bản ghi của 1 SV (dùng khi cần xoá SV triệt để)
void remove_all_exam_of_sv(PTRExamLog& head, const char* masv_ci);

#endif // EXAMLOG_H
