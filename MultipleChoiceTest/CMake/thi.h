// thi.h
#ifndef THI_H
#define THI_H

#include "structs.h"
#include "monhoc_avl.h"
#include "examlog.h"

// Tổ chức thi trắc nghiệm cho SV:
// - masv_ci: mã SV đã normalize
// - mamh_ci: mã môn đã normalize
// - soCau: số câu muốn thi (1..số câu của môn)
// - thoiGianPhut: thời lượng phút
// Kết quả: chấm điểm, cập nhật ExamLog + upsert điểm của SV tương ứng.
// Trả 0 OK; 1 ko tìm thấy SV; 2 ko tìm thấy môn; 3 số câu không hợp lệ; 4 môn không đủ câu.
int thuc_hien_thi(DS_Lop& ds, PTRMH root, PTRExamLog& logs,
                  const char* masv_ci, const char* mamh_ci,
                  int soCau, int thoiGianPhut);

// In chi tiết bài thi của 1 SV/môn (nếu có)
void in_chi_tiet_bai_thi(PTRExamLog logs, const char* masv_ci, const char* mamh_ci);

// In bảng điểm của 1 lớp theo từng môn trong cây (nếu SV chưa thi môn X -> "CHUA")
void in_bang_diem_lop(const DS_Lop& ds, const char* malop_ci, PTRMH root);

#endif // THI_H
