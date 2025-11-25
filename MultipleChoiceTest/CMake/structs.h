#ifndef STRUCTS_H
#define STRUCTS_H

#include <cstddef>   // để dùng nullptr (cũng an toàn cho size_t nếu cần)
#include <iostream>
#include <string>
#include <fstream>
#include <cstring>

// Hằng số
const int MAX_LOP = 10000;

// ================== CÂU HỎI (DSLK đơn) ==================
struct CauHoi {
    int  id;             // Id (số nguyên tự động tăng)
    char noidung[501];   // Nội dung
    char A[201];
    char B[201];
    char C[201];
    char D[201];
    char dapan;          // 'A'|'B'|'C'|'D'
};

struct NodeCH {
    CauHoi  data;
    NodeCH* next;
};
typedef NodeCH* PTRCH;

// ================== MÔN HỌC (AVL) ==================
struct MonHoc {
    char  mamh[16];      //C15
    char  tenmh[51];
    PTRCH FirstCHT = nullptr; // danh sách câu hỏi
    int   height;        // chiều cao node AVL
};

struct NodeMH {
    MonHoc  data;
    NodeMH* left;
    NodeMH* right;
};
typedef NodeMH* PTRMH;

// ================== ĐIỂM THI (DSLK đơn) ==================
struct DiemThi {
    char  mamh[16];      // C15
    float diem;          // làm tròn 1 số sau dấu chấm ở nơi khác
};

struct NodeDiem {
    DiemThi   data;
    NodeDiem* next;
};
typedef NodeDiem* PTRDiemThi;

// ================== SINH VIÊN (DSLK đơn) ==================
struct SinhVien {
    char        masv[16];    // C15
    char        ho[51];
    char        ten[16];
    char        phai[4];     // "NAM"/"NU"
    char        password[21];
    PTRDiemThi  ds_diemthi = nullptr;
};

struct NodeSV {
    SinhVien data;
    NodeSV*  next;
};
typedef NodeSV* PTRSV;

// ================== LỚP (mảng con trỏ) ==================
struct Lop {
    char  malop[16];     // C15
    char  tenlop[51];
    PTRSV FirstSV = nullptr;
};

struct DS_Lop {
    int  n = 0;
    Lop* nodes[MAX_LOP]; // mảng con trỏ lớp
};

#endif // STRUCTS_H
