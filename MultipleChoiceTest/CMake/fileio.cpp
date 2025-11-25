// fileio.cpp
#include "fileio.h"
#include "string_utils.h"
#include "ds_ops.h"
#include "cauhoi.h"
#include "diemthi.h"
#include <cstdio>
#include <cstdlib>

// ====== Helpers ======
static bool open_read(const char* path, FILE*& f) {
    f = std::fopen(path, "r");
    if (!f) return false;
    return true;
}
static bool open_write(const char* path, FILE*& f) {
    f = std::fopen(path, "w");
    if (!f) return false;
    return true;
}
static bool read_line(FILE* f, char* buf, int cap) {
    if (!std::fgets(buf, cap, f)) return false;
    chomp_line(buf);
    return true;
}

// ===================================================================
// 1) Lop.txt
// Format:
// SoLop
// MALOP_1, TENLOP_1, SoSV_1
// MALOP_2, TENLOP_2, SoSV_2
// ...
// ===================================================================

bool doc_lop_txt(const char* path, DS_Lop& ds) {
    init_ds_lop(ds);
    FILE* f = NULL;
    if (!open_read(path, f)) {
        // Không có file -> coi như rỗng
        return true;
    }
    char line[2048];
    if (!read_line(f, line, sizeof(line))) {
        std::fclose(f);
        return true;
    }
    strip_bom_inplace(line);
    int soLop = std::atoi(line);

    for (int i = 0; i < soLop; ++i) {
        if (!read_line(f, line, sizeof(line))) break;
        char* toks[4];
        int c = split_csv_inplace(line, toks, 4);
        // mong đợi: MALOP, TENLOP, SoSV (SoSV có thể thiếu)
        if (c < 2) continue;
        const char* malop = toks[0];
        const char* tenlop = toks[1];

        // Thêm lớp; SoSV chỉ dùng để tham khảo nên có thể bỏ qua
        them_lop(ds, malop, tenlop);
    }
    std::fclose(f);
    return true;
}

bool ghi_lop_txt(const char* path, const DS_Lop& ds) {
    FILE* f = NULL;
    if (!open_write(path, f)) return false;

    std::fprintf(f, "%d\n", ds.n);
    for (int i = 0; i < ds.n; ++i) {
        const Lop* lp = ds.nodes[i];
        int soSV = so_sinh_vien_trong_lop(lp);
        // 1 dòng / lớp
        std::fprintf(f, "%s, %s, %d\n",
                     lp->malop,
                     lp->tenlop,
                     soSV);
    }
    std::fclose(f);
    return true;
}

// ===================================================================
// 2) SinhVien.txt
// Format:
// SoSV
// MALOP, MASV, HO, TEN, PHAI, PASSWORD
// ...
// ===================================================================

bool doc_sinhvien_txt(const char* path, DS_Lop& ds) {
    FILE* f = NULL;
    if (!open_read(path, f)) {
        // Không có file -> DS lớp vẫn giữ nguyên, chỉ là chưa có SV
        return true;
    }
    char line[4096];
    if (!read_line(f, line, sizeof(line))) {
        std::fclose(f);
        return true;
    }
    strip_bom_inplace(line);
    int soSV = std::atoi(line);

    for (int i = 0; i < soSV; ++i) {
        if (!read_line(f, line, sizeof(line))) break;
        char* toks[8];
        int c = split_csv_inplace(line, toks, 8);
        if (c < 6) continue;
        // MALOP, MASV, HO, TEN, PHAI, PASSWORD
        them_sv_vao_ds(ds, toks[0], toks[1], toks[2], toks[3], toks[4], toks[5]);
    }

    std::fclose(f);
    return true;
}

bool ghi_sinhvien_txt(const char* path, const DS_Lop& ds) {
    // Đếm tổng số SV
    int soSV = 0;
    for (int i = 0; i < ds.n; ++i) {
        const Lop* lp = ds.nodes[i];
        for (PTRSV p = lp->FirstSV; p; p = p->next) ++soSV;
    }

    FILE* f = NULL;
    if (!open_write(path, f)) return false;

    std::fprintf(f, "%d\n", soSV);
    for (int i = 0; i < ds.n; ++i) {
        const Lop* lp = ds.nodes[i];
        for (PTRSV p = lp->FirstSV; p; p = p->next) {
            std::fprintf(f, "%s, %s, %s, %s, %s, %s\n",
                         lp->malop,
                         p->data.masv,
                         p->data.ho,
                         p->data.ten,
                         p->data.phai,
                         p->data.password);
        }
    }
    std::fclose(f);
    return true;
}

// ===================================================================
// 3) MonHoc.txt
// Format:
// SoMon
// MAMH, TENMH
// ...
// ===================================================================

bool doc_monhoc_txt(const char* path, PTRMH& root) {
    init_avl(root);
    FILE* f = NULL;
    if (!open_read(path, f)) return true;

    char line[2048];
    if (!read_line(f, line, sizeof(line))) {
        std::fclose(f);
        return true;
    }
    strip_bom_inplace(line);
    int soMon = std::atoi(line);

    for (int i = 0; i < soMon; ++i) {
        if (!read_line(f, line, sizeof(line))) break;
        char* tok[3];
        int c = split_csv_inplace(line, tok, 3);
        if (c < 2) continue;
        // MAMH, TENMH
        insert_monhoc(root, tok[0], tok[1]);
    }
    std::fclose(f);
    return true;
}

bool ghi_monhoc_txt(const char* path, PTRMH root) {
    // Đếm số môn
    int count = 0;
    struct LocalCount {
        static void cnt(PTRMH r, int& c) {
            if (!r) return;
            cnt(r->left, c);
            ++c;
            cnt(r->right, c);
        }
    };
    LocalCount::cnt(root, count);

    FILE* f = NULL;
    if (!open_write(path, f)) return false;
    std::fprintf(f, "%d\n", count);

    // In-order
    struct LocalOut {
        static void out(PTRMH r, FILE* f) {
            if (!r) return;
            out(r->left, f);
            std::fprintf(f, "%s, %s\n", r->data.mamh, r->data.tenmh);
            out(r->right, f);
        }
    };
    LocalOut::out(root, f);
    std::fclose(f);
    return true;
}

// ===================================================================
// 4) CauHoi.txt
// Format:
// TongCau
// id, mamh, noidung, A, B, C, D, dapan
// ...
// ===================================================================

bool doc_cauhoi_txt(const char* path, PTRMH root) {
    FILE* f = NULL;
    if (!open_read(path, f)) return true;

    char line[4096];
    if (!read_line(f, line, sizeof(line))) {
        std::fclose(f);
        return true;
    }
    strip_bom_inplace(line);
    int total = std::atoi(line);

    for (int i = 0; i < total; ++i) {
        if (!read_line(f, line, sizeof(line))) break;
        char* toks[8];
        int c = split_csv_inplace(line, toks, 8);
        if (c < 8) continue;

        CauHoi ch{};
        ch.id = std::atoi(toks[0]);

        char mamh[16];
        su_strncpy(mamh, toks[1], 16);
        normalize_code(mamh); // chuẩn mã để tìm trong AVL

        su_strncpy(ch.noidung, toks[2], 501);
        su_strncpy(ch.A,        toks[3], 201);
        su_strncpy(ch.B,        toks[4], 201);
        su_strncpy(ch.C,        toks[5], 201);
        su_strncpy(ch.D,        toks[6], 201);
        ch.dapan = toks[7][0];

        PTRMH mh = find_monhoc(root, mamh);
        if (!mh) {
            // Không có môn tương ứng -> bỏ qua câu này
            continue;
        }
        add_cau_hoi(mh->data.FirstCHT, ch);
    }

    std::fclose(f);
    return true;
}

bool ghi_cauhoi_txt(const char* path, PTRMH root) {
    // Đếm tổng số câu
    int total = 0;
    struct LocalCount {
        static void count(PTRMH r, int& t) {
            if (!r) return;
            count(r->left, t);
            for (PTRCH p = r->data.FirstCHT; p; p = p->next) ++t;
            count(r->right, t);
        }
    };
    LocalCount::count(root, total);

    FILE* f = NULL;
    if (!open_write(path, f)) return false;
    std::fprintf(f, "%d\n", total);

    // Ghi từng câu trên 1 dòng
    struct LocalOut {
        static void out(PTRMH r, FILE* f) {
            if (!r) return;
            out(r->left, f);
            for (PTRCH p = r->data.FirstCHT; p; p = p->next) {
                std::fprintf(f, "%d, %s, %s, %s, %s, %s, %s, %c\n",
                             p->data.id,
                             r->data.mamh,
                             p->data.noidung,
                             p->data.A,
                             p->data.B,
                             p->data.C,
                             p->data.D,
                             p->data.dapan);
            }
            out(r->right, f);
        }
    };
    LocalOut::out(root, f);
    std::fclose(f);
    return true;
}

// ===================================================================
// 5) SinhVienDaThi.txt
// Giữ nguyên format cũ (nhiều dòng / record)
// ===================================================================
//
// SoRecord
// (cho mỗi record)
//   masv
//   soCau
//   lặp soCau lần (9 dòng/câu): mamh, id, noidung, A, B, C, D, dapan, da_chon
//   soCauDung
//   diem
//

bool doc_sinhvien_dathi_txt(const char* path, PTRExamLog& logs, DS_Lop& ds) {
    init_examlog(logs);
    FILE* f = NULL;
    if (!open_read(path, f)) return true;

    char line[4096];
    if (!read_line(f, line, sizeof(line))) {
        std::fclose(f);
        return true;
    }
    strip_bom_inplace(line);
    int soRec = std::atoi(line);

    for (int r = 0; r < soRec; ++r) {
        char masv[32];
        if (!read_line(f, masv, sizeof(masv))) break;
        trim_inplace(masv);

        char so[64];
        if (!read_line(f, so, sizeof(so))) break;
        int soCau = std::atoi(so);

        ExamQItem* arr = new ExamQItem[soCau];
        char mamh_first[16];
        mamh_first[0] = '\0';

        for (int i = 0; i < soCau; ++i) {
            char mamh[32];
            if (!read_line(f, mamh, sizeof(mamh))) {
                soCau = i;
                break;
            }
            trim_inplace(mamh);
            if (i == 0) su_strncpy(mamh_first, mamh, 16);

            char idbuf[64];
            if (!read_line(f, idbuf, sizeof(idbuf))) {
                soCau = i;
                break;
            }
            arr[i].id = std::atoi(idbuf);

            if (!read_line(f, arr[i].noidung, sizeof(arr[i].noidung))) { soCau = i; break; }
            if (!read_line(f, arr[i].A,       sizeof(arr[i].A)))       { soCau = i; break; }
            if (!read_line(f, arr[i].B,       sizeof(arr[i].B)))       { soCau = i; break; }
            if (!read_line(f, arr[i].C,       sizeof(arr[i].C)))       { soCau = i; break; }
            if (!read_line(f, arr[i].D,       sizeof(arr[i].D)))       { soCau = i; break; }

            char dd[8];
            if (!read_line(f, dd, sizeof(dd))) { soCau = i; break; }
            arr[i].dapan = dd[0];

            char dc[8];
            if (!read_line(f, dc, sizeof(dc))) { soCau = i; break; }
            arr[i].da_chon = dc[0];

            su_strncpy(arr[i].mamh, mamh, 16);
        }

        char scd[64];
        if (!read_line(f, scd, sizeof(scd))) {
            delete[] arr;
            break;
        }
        int soCauDung = std::atoi(scd);

        char d[64];
        if (!read_line(f, d, sizeof(d))) {
            delete[] arr;
            break;
        }
        float diem = (float)std::atof(d);

        // upsert examlog
        upsert_exam(logs, masv, mamh_first, arr, soCau, soCauDung, diem);

        // Cập nhật điểm vào ds (SV có thể thi nhiều môn => gộp)
        for (int i = 0; i < ds.n; ++i) {
            Lop* lp = ds.nodes[i];
            for (PTRSV p = lp->FirstSV; p; p = p->next) {
                if (su_stricmp(p->data.masv, masv) == 0) {
                    upsert_diem(p->data.ds_diemthi, mamh_first, diem);
                }
            }
        }

        delete[] arr;
    }
    std::fclose(f);
    return true;
}

bool ghi_sinhvien_dathi_txt(const char* path, PTRExamLog logs) {
    // Đếm số record
    int cnt = 0;
    for (ExamRecord* p = logs; p; p = p->next) ++cnt;

    FILE* f = NULL;
    if (!open_write(path, f)) return false;
    std::fprintf(f, "%d\n", cnt);

    for (ExamRecord* p = logs; p; p = p->next) {
        std::fprintf(f, "%s\n", p->masv);
        std::fprintf(f, "%d\n", p->soCau);
        for (int i = 0; i < p->soCau; ++i) {
            std::fprintf(f, "%s\n", p->items[i].mamh);
            std::fprintf(f, "%d\n", p->items[i].id);
            std::fprintf(f, "%s\n", p->items[i].noidung);
            std::fprintf(f, "%s\n", p->items[i].A);
            std::fprintf(f, "%s\n", p->items[i].B);
            std::fprintf(f, "%s\n", p->items[i].C);
            std::fprintf(f, "%s\n", p->items[i].D);
            std::fprintf(f, "%c\n", p->items[i].dapan);
            std::fprintf(f, "%c\n", p->items[i].da_chon);
        }
        std::fprintf(f, "%d\n", p->soCauDung);
        std::fprintf(f, "%.1f\n", p->diem);
    }
    std::fclose(f);
    return true;
}
