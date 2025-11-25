// menu.cpp
#include "menu.h"
#include "fileio.h"
#include "ds_ops.h"
#include "monhoc_avl.h"
#include "cauhoi.h"
#include "thi.h"
#include "console.h"
#include "string_utils.h"
#include "examlog.h"
#include "rng.h"
#include <cstdio>
#include <cstdlib>
#include "diemthi.h"
// Đường dẫn file TXT
static const char* PATH_LOP  = "Lop.txt";
static const char* PATH_SV   = "SinhVien.txt";
static const char* PATH_MON  = "MonHoc.txt";
static const char* PATH_CH   = "CauHoi.txt";
static const char* PATH_SVDT = "SinhVienDaThi.txt";

// Biến trạng thái toàn cục
static DS_Lop    g_dslop;
static PTRMH     g_rootMH = NULL;
static PTRExamLog g_logs  = NULL;

// Lưu tất cả
static void save_all() {
    if (!ghi_lop_txt(PATH_LOP, g_dslop))        printf("Loi ghi %s\n", PATH_LOP);
    if (!ghi_sinhvien_txt(PATH_SV, g_dslop))    printf("Loi ghi %s\n", PATH_SV);
    if (!ghi_monhoc_txt(PATH_MON, g_rootMH))    printf("Loi ghi %s\n", PATH_MON);
    if (!ghi_cauhoi_txt(PATH_CH, g_rootMH))     printf("Loi ghi %s\n", PATH_CH);
    if (!ghi_sinhvien_dathi_txt(PATH_SVDT, g_logs)) printf("Loi ghi %s\n", PATH_SVDT);
    printf("Da luu tat ca.\n");
}

// Nạp tất cả
static void load_all() {
    free_ds_lop(g_dslop);
    free_all_monhoc(g_rootMH);
    free_examlog(g_logs);

    // Đọc lớp trước, rồi sinh viên (vì SinhVien.txt cần MALOP đã có sẵn)
    doc_lop_txt(PATH_LOP, g_dslop);
    doc_sinhvien_txt(PATH_SV, g_dslop);
    doc_monhoc_txt(PATH_MON, g_rootMH);
    doc_cauhoi_txt(PATH_CH, g_rootMH);
    doc_sinhvien_dathi_txt(PATH_SVDT, g_logs, g_dslop);
}

static void menu_quanly_lop() {
    while (1) {
        clearScreen();
        printf("=== QUAN LY LOP ===\n");
        printf("1. Them lop\n2. Xoa lop\n3. In ds lop\n4. Quay lai\nChon: ");
        int c; std::scanf("%d", &c); while (getchar() != '\n');
        if (c == 1) {
            char ml[64], tl[128];
            printf("Nhap ma lop: "); std::fgets(ml, sizeof(ml), stdin); chomp_line(ml);
            printf("Nhap ten lop: "); std::fgets(tl, sizeof(tl), stdin); chomp_line(tl);
            int r = them_lop(g_dslop, ml, tl);
            if (r == 0) printf("OK\n"); else printf("That bai (code=%d)\n", r);
            system("pause");
        } else if (c == 2) {
            char ml[64];
            printf("Nhap ma lop can xoa: "); std::fgets(ml, sizeof(ml), stdin); chomp_line(ml);
            char t[16]; su_strncpy(t, ml, 16); normalize_code(t);
            if (!confirm_dialog("Ban chac chan xoa lop? (se chan neu co SV co diem)")) continue;
            if (xoa_lop(g_dslop, t)) printf("Da xoa.\n"); else printf("Khong xoa duoc (co the lop co SV da thi).\n");
            system("pause");
        } else if (c == 3) {
            clearScreen();
            printf("So lop: %d\n", g_dslop.n);
            for (int i = 0; i < g_dslop.n; ++i) {
                const Lop* lp = g_dslop.nodes[i];
                printf("%d) %s - %s  (SV: %d)\n",
                       i + 1, lp->malop, lp->tenlop, so_sinh_vien_trong_lop(lp));
            }
            system("pause");
        } else if (c == 4) return;
    }
}

static void menu_quanly_sv() {
    while (1) {
        clearScreen();
        printf("=== QUAN LY SINH VIEN ===\n");
        printf("1. Nhap SV vao lop\n2. Sua SV\n3. Xoa SV\n4. In SV cua lop\n5. Quay lai\nChon: ");
        int c; std::scanf("%d", &c); while (getchar() != '\n');
        if (c == 1) {
            char ml[64], ms[64], ho[128], ten[64], phai[16], pw[64];
            printf("Ma lop: "); std::fgets(ml, sizeof(ml), stdin); chomp_line(ml);
            printf("MASV: "); std::fgets(ms, sizeof(ms), stdin); chomp_line(ms);
            printf("Ho: "); std::fgets(ho, sizeof(ho), stdin); chomp_line(ho);
            printf("Ten: "); std::fgets(ten, sizeof(ten), stdin); chomp_line(ten);
            printf("Phai (NAM/NU): "); std::fgets(phai, sizeof(phai), stdin); chomp_line(phai);
            printf("Password (4-20): "); std::fgets(pw, sizeof(pw), stdin); chomp_line(pw);
            int r = them_sv_vao_ds(g_dslop, ml, ms, ho, ten, phai, pw);
            if (r == 0) printf("OK\n"); else printf("That bai (code=%d)\n", r);
            system("pause");
        } else if (c == 2) {
            char ml[64], ms[64], ho[128], ten[64], phai[16], pw[64];
            printf("Ma lop: "); std::fgets(ml, sizeof(ml), stdin); chomp_line(ml);
            char tml[16]; su_strncpy(tml, ml, 16); normalize_code(tml);
            Lop* lop = get_lop(g_dslop, tml);
            if (!lop) { printf("Khong co lop\n"); system("pause"); continue; }
            printf("MASV: "); std::fgets(ms, sizeof(ms), stdin); chomp_line(ms);
            char tms[16]; su_strncpy(tms, ms, 16); normalize_code(tms);
            printf("Ho moi: "); std::fgets(ho, sizeof(ho), stdin); chomp_line(ho);
            printf("Ten moi: "); std::fgets(ten, sizeof(ten), stdin); chomp_line(ten);
            printf("Phai moi: "); std::fgets(phai, sizeof(phai), stdin); chomp_line(phai);
            printf("Password moi: "); std::fgets(pw, sizeof(pw), stdin); chomp_line(pw);
            int r = sua_sv_trong_lop(lop, tms, ho, ten, phai, pw);
            if (r == 0) printf("OK\n"); else printf("That bai (code=%d)\n", r);
            system("pause");
        } else if (c == 3) {
            char ml[64], ms[64];
            printf("Ma lop: "); std::fgets(ml, sizeof(ml), stdin); chomp_line(ml);
            printf("MASV: "); std::fgets(ms, sizeof(ms), stdin); chomp_line(ms);
            char tml[16]; su_strncpy(tml, ml, 16); normalize_code(tml);
            char tms[16]; su_strncpy(tms, ms, 16); normalize_code(tms);
            Lop* lop = get_lop(g_dslop, tml);
            if (!lop) { printf("Khong co lop\n"); system("pause"); continue; }
            if (!confirm_dialog("Xoa SV? (se chan neu SV da thi)")) { continue; }
            if (xoa_sv_khoi_lop(lop, tms)) {
                // Nếu muốn xoá luôn bài thi thì có thể gọi:
                // remove_all_exam_of_sv(g_logs, tms);
                printf("Da xoa.\n");
            } else {
                printf("Khong xoa duoc (SV co diem/bai thi), hoac khong ton tai.\n");
            }
            system("pause");
        } else if (c == 4) {
            char ml[64]; printf("Ma lop: "); std::fgets(ml, sizeof(ml), stdin); chomp_line(ml);
            char tml[16]; su_strncpy(tml, ml, 16); normalize_code(tml);
            Lop* lop = get_lop(g_dslop, tml);
            if (!lop) { printf("Khong co lop\n"); system("pause"); continue; }
            printf("=== SV cua lop %s - %s ===\n", lop->malop, lop->tenlop);
            for (PTRSV p = lop->FirstSV; p; p = p->next) {
                printf("%-12s | %-20s | %-4s | (mon da thi: %d)\n",
                       p->data.masv, p->data.ho, p->data.phai, dem_diem(p->data.ds_diemthi));
            }
            system("pause");
        } else if (c == 5) return;
    }
}

static void menu_quanly_monhoc() {
    while (1) {
        clearScreen();
        printf("=== QUAN LY MON HOC (AVL) ===\n");
        printf("1. Them MH\n2. Sua ten MH\n3. Xoa MH (chan neu con cau hoi)\n4. Liet ke (in-order)\n5. Quay lai\nChon: ");
        int c; std::scanf("%d", &c); while (getchar() != '\n');
        if (c == 1) {
            char mm[64], tn[128];
            printf("Ma MH: "); std::fgets(mm, sizeof(mm), stdin); chomp_line(mm);
            printf("Ten MH: "); std::fgets(tn, sizeof(tn), stdin); chomp_line(tn);
            int r = insert_monhoc(g_rootMH, mm, tn);
            if (r == 0) printf("OK.\n"); else printf("Trung ma.\n");
            system("pause");
        } else if (c == 2) {
            char mm[64], tn[128];
            printf("Ma MH: "); std::fgets(mm, sizeof(mm), stdin); chomp_line(mm);
            printf("Ten MH moi: "); std::fgets(tn, sizeof(tn), stdin); chomp_line(tn);
            char tmm[16]; su_strncpy(tmm, mm, 16); normalize_code(tmm);
            int r = update_monhoc_name(g_rootMH, tmm, tn);
            if (r == 0) printf("OK.\n"); else printf("Khong tim thay.\n");
            system("pause");
        } else if (c == 3) {
            char mm[64];
            printf("Ma MH can xoa: "); std::fgets(mm, sizeof(mm), stdin); chomp_line(mm);
            char tmm[16]; su_strncpy(tmm, mm, 16); normalize_code(tmm);
            if (!confirm_dialog("Xac nhan xoa MH (se CHAN neu con cau hoi)")) continue;
            if (delete_monhoc_safe(g_rootMH, tmm)) printf("Da xoa.\n");
            else printf("Khong the xoa (MH khong ton tai hoac con cau hoi).\n");
            system("pause");
        } else if (c == 4) {
            printf("=== Danh sach mon hoc (in-order) ===\n");
            struct Local {
                static void show(const MonHoc& m) {
                    printf("%s - %s (so cau: %d)\n", m.mamh, m.tenmh, dem_cau(m.FirstCHT));
                }
            };
            traverse_inorder(g_rootMH, Local::show);
            system("pause");
        } else if (c == 5) return;
    }
}

static void menu_quanly_cauhoi() {
    while (1) {
        clearScreen();
        printf("=== QUAN LY CAU HOI THEO MON ===\n");
        printf("1. Them cau hoi\n2. Xoa cau hoi (chan neu da thi)\n3. In ds cau cua 1 mon\n4. Quay lai\nChon: ");
        int c; std::scanf("%d", &c); while (getchar() != '\n');
        if (c == 1) {
            char mm[64];
            printf("Ma MH: "); std::fgets(mm, sizeof(mm), stdin); chomp_line(mm);
            char tmm[16]; su_strncpy(tmm, mm, 16); normalize_code(tmm);
            PTRMH mh = find_monhoc(g_rootMH, tmm);
            if (!mh) { printf("Khong co mon.\n"); system("pause"); continue; }
            CauHoi ch; ch.id = new_question_id_unique(g_rootMH);
            printf("Noi dung: "); std::fgets(ch.noidung, sizeof(ch.noidung), stdin); chomp_line(ch.noidung);
            printf("Phuong an A: "); std::fgets(ch.A, sizeof(ch.A), stdin); chomp_line(ch.A);
            printf("Phuong an B: "); std::fgets(ch.B, sizeof(ch.B), stdin); chomp_line(ch.B);
            printf("Phuong an C: "); std::fgets(ch.C, sizeof(ch.C), stdin); chomp_line(ch.C);
            printf("Phuong an D: "); std::fgets(ch.D, sizeof(ch.D), stdin); chomp_line(ch.D);
            char dd[8];
            do {
                printf("Dap an dung (A/B/C/D): "); std::fgets(dd, sizeof(dd), stdin); chomp_line(dd);
                ch.dapan = dd[0];
                if (ch.dapan >= 'a' && ch.dapan <= 'z') ch.dapan = (char)(ch.dapan - 'a' + 'A');
            } while (!(ch.dapan=='A'||ch.dapan=='B'||ch.dapan=='C'||ch.dapan=='D'));
            add_cau_hoi(mh->data.FirstCHT, ch);
            printf("Da them (id=%d).\n", ch.id);
            system("pause");
        } else if (c == 2) {
            char mm[64]; printf("Ma MH: "); std::fgets(mm, sizeof(mm), stdin); chomp_line(mm);
            char tmm[16]; su_strncpy(tmm, mm, 16); normalize_code(tmm);
            PTRMH mh = find_monhoc(g_rootMH, tmm);
            if (!mh) { printf("Khong co mon.\n"); system("pause"); continue; }
            char idbuf[64]; printf("ID cau: "); std::fgets(idbuf, sizeof(idbuf), stdin); chomp_line(idbuf);
            int id = std::atoi(idbuf);
            // chặn xoá nếu câu đã xuất hiện trong examlog
            if (examlog_contains_question(g_logs, tmm, id)) {
                printf("Khong the xoa: cau da duoc dung trong bai thi.\n");
            } else {
                if (remove_cau_by_id(mh->data.FirstCHT, id)) printf("Da xoa.\n");
                else printf("Khong tim thay id.\n");
            }
            system("pause");
        } else if (c == 3) {
            char mm[64]; printf("Ma MH: "); std::fgets(mm, sizeof(mm), stdin); chomp_line(mm);
            char tmm[16]; su_strncpy(tmm, mm, 16); normalize_code(tmm);
            PTRMH mh = find_monhoc(g_rootMH, tmm);
            if (!mh) { printf("Khong co mon.\n"); system("pause"); continue; }
            printf("=== Cau hoi cua %s - %s ===\n", mh->data.mamh, mh->data.tenmh);
            for (PTRCH p = mh->data.FirstCHT; p; p = p->next) {
                printf("ID=%d | %s (DA: %c)\n", p->data.id, p->data.noidung, p->data.dapan);
            }
            system("pause");
        } else if (c == 4) return;
    }
}

static void menu_thi_sv() {
    char ms[64], pw[64];
    printf("=== DANG NHAP SINH VIEN ===\nMASV: "); std::fgets(ms, sizeof(ms), stdin); chomp_line(ms);
    printf("Password: "); std::fgets(pw, sizeof(pw), stdin); chomp_line(pw);
    char tms[16]; su_strncpy(tms, ms, 16); normalize_code(tms);
    // Kiểm tra tài khoản SV
    PTRSV sv = NULL; Lop* lop = NULL;
    for (int i = 0; i < g_dslop.n; ++i) {
        for (PTRSV p = g_dslop.nodes[i]->FirstSV; p; p = p->next) {
            if (su_stricmp(p->data.masv, tms) == 0 && su_strcmp(p->data.password, pw) == 0) {
                sv = p; lop = g_dslop.nodes[i]; break;
            }
        }
        if (sv) break;
    }
    if (!sv) { printf("Sai MASV/password.\n"); system("pause"); return; }

    while (1) {
        clearScreen();
        printf("=== SINH VIEN: %s - %s (%s) ===\n",
               sv->data.masv, sv->data.ho, lop->tenlop);
        printf("1. Thi trac nghiem\n2. Xem diem cac mon\n3. Xem chi tiet bai thi 1 mon\n4. Quay lai\nChon: ");
        int c; std::scanf("%d", &c); while (getchar() != '\n');
        if (c == 1) {
            char mm[64]; printf("Ma MH: "); std::fgets(mm, sizeof(mm), stdin); chomp_line(mm);
            char tmm[16]; su_strncpy(tmm, mm, 16); normalize_code(tmm);
            char sbuf[64], pbuf[64];
            printf("So cau: "); std::fgets(sbuf, sizeof(sbuf), stdin); chomp_line(sbuf);
            printf("So phut: "); std::fgets(pbuf, sizeof(pbuf), stdin); chomp_line(pbuf);
            int soCau = std::atoi(sbuf), soPhut = std::atoi(pbuf);
            int r = thuc_hien_thi(g_dslop, g_rootMH, g_logs, sv->data.masv, tmm, soCau, soPhut);
            if (r != 0) printf("Khong the thi (code=%d)\n", r);
            system("pause");
        } else if (c == 2) {
            printf("=== DIEM CUA BAN ===\n");
            for (PTRDiemThi d = sv->data.ds_diemthi; d; d = d->next) {
                PTRMH mh = find_monhoc(g_rootMH, d->data.mamh);
                const char* ten = (mh ? mh->data.tenmh : "(?)");
                printf("%s - %-20s : %.1f\n", d->data.mamh, ten, d->data.diem);
            }
            system("pause");
        } else if (c == 3) {
            char mm[64]; printf("Ma MH: "); std::fgets(mm, sizeof(mm), stdin); chomp_line(mm);
            char tmm[16]; su_strncpy(tmm, mm, 16); normalize_code(tmm);
            in_chi_tiet_bai_thi(g_logs, sv->data.masv, tmm);
            system("pause");
        } else if (c == 4) return;
    }
}

static void menu_gv() {
    while (1) {
        clearScreen();
        printf("=== QUAN TRI (GV) ===\n");
        printf("1. Quan ly Lop\n2. Quan ly Sinh Vien\n3. Quan ly Mon hoc (AVL)\n4. Quan ly Cau hoi\n5. In bang diem 1 lop\n6. Luu tat ca (Ctrl+S)\n7. Tai lai tu TXT (F5)\n8. Quay lai\nChon: ");
        int c; std::scanf("%d", &c); while (getchar() != '\n');
        if (c == 1) menu_quanly_lop();
        else if (c == 2) menu_quanly_sv();
        else if (c == 3) menu_quanly_monhoc();
        else if (c == 4) menu_quanly_cauhoi();
        else if (c == 5) {
            char ml[64]; printf("Ma lop: "); std::fgets(ml, sizeof(ml), stdin); chomp_line(ml);
            char tml[16]; su_strncpy(tml, ml, 16); normalize_code(tml);
            in_bang_diem_lop(g_dslop, tml, g_rootMH);
            system("pause");
        }
        else if (c == 6) { save_all(); system("pause"); }
        else if (c == 7) { load_all(); printf("Da tai lai.\n"); system("pause"); }
        else if (c == 8) return;
    }
}

void run_console_app() {
    rng_seed_once();
    load_all();

    while (1) {
        clearScreen();
        printf("=== HE THONG THI TRAC NGHIEM (Console) ===\n");
        printf("1. Dang nhap GV (tai khoan: GV / GV)\n2. Dang nhap SV\n3. Luu tat ca (Ctrl+S)\n4. Tai lai TXT (F5)\n5. Thoat\nChon: ");
        int c; std::scanf("%d", &c); while (getchar() != '\n');
        if (c == 1) {
            char u[64], p[64];
            printf("User: "); std::fgets(u, sizeof(u), stdin); chomp_line(u);
            printf("Pass: "); std::fgets(p, sizeof(p), stdin); chomp_line(p);
            if (su_stricmp(u, "GV") == 0 && su_stricmp(p, "GV") == 0) menu_gv();
            else { printf("Sai tai khoan GV.\n"); system("pause"); }
        } else if (c == 2) {
            menu_thi_sv();
        } else if (c == 3) { save_all(); system("pause"); }
        else if (c == 4) { load_all(); printf("Da tai lai.\n"); system("pause"); }
        else if (c == 5) {
            if (confirm_dialog("Ban co muon luu truoc khi thoat")) save_all();
            break;
        }
    }

    // cleanup
    save_all();
    free_ds_lop(g_dslop);
    free_all_monhoc(g_rootMH);
    free_examlog(g_logs);
}
