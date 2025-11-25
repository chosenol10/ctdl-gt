// diemthi.h
#ifndef DIEMTHI_H
#define DIEMTHI_H
#include "structs.h"

// Tìm điểm theo mã môn trong DSLK điểm
PTRDiemThi tim_diem(PTRDiemThi head, const char* mamh_ci);

// Thêm/sửa (upsert) điểm: nếu đã có môn -> ghi đè điểm; nếu chưa có -> chèn đầu.
void upsert_diem(PTRDiemThi& head, const char* mamh_ci, float diem);

// Đếm số môn đã có điểm
int dem_diem(PTRDiemThi head);

// Giải phóng DSLK điểm
void free_ds_diemthi(PTRDiemThi& head);

#endif // DIEMTHI_H
