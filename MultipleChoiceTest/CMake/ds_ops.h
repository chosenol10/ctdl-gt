// ds_ops.h
#ifndef DS_OPS_H
#define DS_OPS_H

#include "structs.h"

// ========== LỚP ==========
void init_ds_lop(DS_Lop& ds);
int find_lop_index(const DS_Lop& ds, const char* malop_ci);  // index hoặc -1
Lop* get_lop(DS_Lop& ds, const char* malop_ci);
const Lop* get_lop_const(const DS_Lop& ds, const char* malop_ci);
int them_lop(DS_Lop& ds, const char* malop, const char* tenlop); // 0 OK; 1 trùng; 2 đầy; 3 mã không hợp lệ
bool xoa_lop(DS_Lop& ds, const char* malop_ci);                   // chặn nếu còn SV có điểm
int so_sinh_vien_trong_lop(const Lop* lop);

// ========== SINH VIÊN ==========
bool exists_sv_global(const DS_Lop& ds, const char* masv_ci);
PTRSV tim_sv_trong_lop(Lop* lop, const char* masv_ci, PTRSV* prev_out = NULL);

// trả về con trỏ SV trong lớp, chỉ dùng để đọc (caller không nên chỉnh sửa data)
PTRSV tim_sv_trong_lop_const(const Lop* lop, const char* masv_ci);

int them_sv_vao_lop(Lop* lop, const SinhVien& sv); // 0 OK; 1 trùng; 2 data ko hợp lệ
int them_sv_vao_ds(DS_Lop& ds,
                   const char* malop,
                   const char* masv,
                   const char* ho,
                   const char* ten,
                   const char* phai,
                   const char* password);          // 0 OK; 1 ko tìm thấy lớp; 2 trùng MASV toàn DS; 3 data ko hợp lệ
int sua_sv_trong_lop(Lop* lop,
                     const char* masv_ci,
                     const char* ho,
                     const char* ten,
                     const char* phai,
                     const char* password);        // 0 OK; 1 ko thấy; 2 data ko hợp lệ
int doi_masv(DS_Lop& ds, Lop* lop, const char* masv_cu_ci, const char* masv_moi);
bool xoa_sv_khoi_lop(Lop* lop, const char* masv_ci); // chặn nếu có điểm?
int dem_sv(PTRSV head);

// ========== GIẢI PHÓNG ==========
void free_list_sv(PTRSV& head);
void free_ds_lop(DS_Lop& ds);

#endif // DS_OPS_H
