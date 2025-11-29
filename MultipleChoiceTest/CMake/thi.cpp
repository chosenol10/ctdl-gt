// thi.cpp (toi uu UX + canh bao thi lai + in ro dung/sai)
#include "thi.h"
#include "ds_ops.h"
#include "cauhoi.h"
#include "console.h"
#include "string_utils.h"
#include "rng.h"
#include "diemthi.h"

#include <cstdio>
#include <cstdlib>
#include <windows.h>

// lam tron 1 chu so thap phan (vd 7.26 -> 7.3)
static float round1(float x) {
    if (x >= 0) return (float)((int)(x * 10 + 0.5f)) / 10.0f;
    return (float)((int)(x * 10 - 0.5f)) / 10.0f;
}

// tim SV theo MASV (da normalize) tren toan he thong
static PTRSV find_sv_global(DS_Lop& ds, const char* masv_ci, Lop** lopFound) {
    if (lopFound) *lopFound = NULL;
    for (int i = 0; i < ds.n; ++i) {
        PTRSV p = ds.nodes[i]->FirstSV;
        for (; p; p = p->next) {
            if (su_stricmp(p->data.masv, masv_ci) == 0) {
                if (lopFound) *lopFound = ds.nodes[i];
                return p;
            }
        }
    }
    return NULL;
}

// gom cau hoi vao mang con tro
static int collect_questions(PTRCH head, PTRCH* arr, int cap) {
    int c = 0;
    for (PTRCH p = head; p && c < cap; p = p->next) arr[c++] = p;
    return c;
}

// tron ngau nhien mang chi so
static void shuffle_indices(int* idx, int n) {
    rng_seed_once();
    for (int i = n - 1; i > 0; --i) {
        int j = rand_in(0, i);
        int t = idx[i]; idx[i] = idx[j]; idx[j] = t;
    }
}

// ------------------------------------------------------
// THUC HIEN THI TRAC NGHIEM
// ------------------------------------------------------
int thuc_hien_thi(DS_Lop& ds, PTRMH root, PTRExamLog& logs,
                  const char* masv_ci, const char* mamh_ci,
                  int soCau, int thoiGianPhut) {
    Lop* lop = NULL;
    PTRSV sv = find_sv_global(ds, masv_ci, &lop);
    if (!sv) return 1;

    PTRMH mh = find_monhoc(root, mamh_ci);
    if (!mh) return 2;

    int n_q = dem_cau(mh->data.FirstCHT);
    if (soCau <= 0) return 3;
    if (soCau > n_q) return 4;

    // === Canh bao thi lai (neu da co bai thi truoc do) ===
    ExamRecord* oldExam = find_exam(logs, masv_ci, mamh_ci);
    if (oldExam) {
        printf("\nBAN DA TUNG THI MON NAY TRUOC DO.\n");
        printf("- Diem hien tai: %.1f\n", oldExam->diem);
        printf("- Thi lai se GHI DE diem va bai thi cu.\n");
        if (!confirm_dialog("Ban co chac muon THI LAI mon nay khong")) {
            printf("Da huy thi mon nay.\n");
            system("pause");
            return 0; // chi huy thi, khong phai loi
        }
    }

    // Tao mang cau hoi
    PTRCH* arr = new PTRCH[n_q];
    int cnt = collect_questions(mh->data.FirstCHT, arr, n_q);

    int* idx = new int[cnt];
    for (int i = 0; i < cnt; ++i) idx[i] = i;
    shuffle_indices(idx, cnt);

    ExamQItem* items = new ExamQItem[soCau];
    for (int i = 0; i < soCau; ++i) {
        PTRCH q = arr[idx[i]];
        items[i].id = q->data.id;
        su_strncpy(items[i].mamh, mh->data.mamh, 16);
        su_strncpy(items[i].noidung, q->data.noidung, 501);
        su_strncpy(items[i].A, q->data.A, 201);
        su_strncpy(items[i].B, q->data.B, 201);
        su_strncpy(items[i].C, q->data.C, 201);
        su_strncpy(items[i].D, q->data.D, 201);
        items[i].dapan   = q->data.dapan;
        items[i].da_chon = '?';
    }

    delete[] arr; arr = NULL;
    delete[] idx; idx = NULL;

    // === Giao dien thi ===
    clearScreen();
    hideCursor();
    printf("THI TRAC NGHIEM - SV: %s  - Lop: %s  - Mon: %s\n",
           sv->data.masv,
           lop ? lop->malop : "(?)",
           mh->data.mamh);
    printf("So cau: %d   Thoi gian: %d phut   (F9: NOP SOM, ESC: HOI NOP BAI)\n",
           soCau, thoiGianPhut);
    printf("---------------------------------------------------------------\n");

    int cur = 0;
    int totalSec = thoiGianPhut * 60;
    ULONGLONG endTick = GetTickCount64() + (ULONGLONG)totalSec * 1000ULL;

    while (1) {
        int remain = (int)((endTick - GetTickCount64()) / 1000ULL);
        if (remain < 0) remain = 0;

        // Dong thong tin cau + thoi gian
        gotoxy(0, 2);
        printf("Cau %d/%d   ", cur + 1, soCau);
        print_mmss_at(25, 2, remain);

        // In noi dung cau hoi (co padding de khong de lai ky tu thua)
        gotoxy(0, 4);
        printf("Noi dung: %-70s\n", items[cur].noidung);
        printf("A) %-70s\n", items[cur].A);
        printf("B) %-70s\n", items[cur].B);
        printf("C) %-70s\n", items[cur].C);
        printf("D) %-70s\n", items[cur].D);
        printf("Chon (A/B/C/D), <- ->: lui/tien, F9: nop som, ESC: hoi nop bai.\n");
        printf("Da chon: %c\n", items[cur].da_chon);

        if (remain == 0) {
            gotoxy(0, 12);
            printf("Het gio! Tu dong nop bai...\n");
            Sleep(1000);
            break;
        }

        int key = readKey();
        if (key == KEY_NONE) { Sleep(60); continue; }

        if (key == KEY_ESC) {
            if (confirm_dialog("Ban co muon NOP BAI ngay bay gio khong")) break;
        } else if (key == KEY_F9) {
            if (confirm_dialog("Ban co muon NOP SOM khong")) break;
        } else if (key == KEY_LEFT) {
            if (cur > 0) --cur;
        } else if (key == KEY_RIGHT || key == KEY_ENTER) {
            if (cur < soCau - 1) ++cur;
        } else {
            if (key >= 'a' && key <= 'z') key = key - 'a' + 'A';
            if (key == 'A' || key == 'B' || key == 'C' || key == 'D') {
                items[cur].da_chon = (char)key;
                if (cur < soCau - 1) ++cur;
            }
        }
    }
    showCursor();

    // Tinh diem
    int dung = 0;
    for (int i = 0; i < soCau; ++i) {
        if (items[i].da_chon == items[i].dapan) ++dung;
    }
    float diem = round1(10.0f * dung / (float)soCau);

    // Luu bai thi + diem
    upsert_exam(logs, sv->data.masv, mh->data.mamh, items, soCau, dung, diem);
    upsert_diem(sv->data.ds_diemthi, mh->data.mamh, diem);

    printf("\nDa NOP BAI.\n");
    printf("- So cau dung: %d/%d\n", dung, soCau);
    printf("- Diem: %.1f\n", diem);
    printf("Ket qua da duoc LUu.\n");
    printf("Ban co the xem lai tai menu:\n");
    printf("- 'Xem diem cac mon'\n");
    printf("- 'Xem chi tiet bai thi 1 mon'\n");
    printf("\nNhan phim bat ky de tiep tuc...");
    _getch();

    delete[] items;
    return 0;
}

// ------------------------------------------------------
// IN CHI TIET BAI THI CUA 1 SV CHO 1 MON
// ------------------------------------------------------
void in_chi_tiet_bai_thi(PTRExamLog logs, const char* masv_ci, const char* mamh_ci) {
    ExamRecord* r = find_exam(logs, masv_ci, mamh_ci);
    if (!r) {
        printf("Khong tim thay bai thi cua SV '%s' - Mon '%s'.\n", masv_ci, mamh_ci);
        return;
    }
    printf("=== CHI TIET BAI THI - SV: %s - MON: %s ===\n", r->masv, r->mamh);
    printf("So cau: %d   Dung: %d   Diem: %.1f\n", r->soCau, r->soCauDung, r->diem);

    for (int i = 0; i < r->soCau; ++i) {
        printf("\nCau %d (id=%d): %s\n", i + 1, r->items[i].id, r->items[i].noidung);
        printf("A) %s\nB) %s\nC) %s\nD) %s\n",
               r->items[i].A, r->items[i].B, r->items[i].C, r->items[i].D);
        const char* kq = (r->items[i].da_chon == r->items[i].dapan) ? "DUNG" : "SAI";
        printf("Chon: %c    Dap an: %c    => %s\n",
               r->items[i].da_chon, r->items[i].dapan, kq);
    }
}

// ------------------------------------------------------
// IN BANG DIEM 1 LOP
// ------------------------------------------------------
static void print_diem_of_sv_for_mon(PTRSV sv, const char* mamh_ci) {
    PTRDiemThi d = sv->data.ds_diemthi;
    while (d) {
        if (su_stricmp(d->data.mamh, mamh_ci) == 0) {
            printf("%5.1f", d->data.diem);
            return;
        }
        d = d->next;
    }
    printf("  CHUA"); // chua thi mon nay
}

static void inorder_print_bangdiem(const DS_Lop& ds, const char* malop_ci, PTRMH r) {
    if (!r) return;
    inorder_print_bangdiem(ds, malop_ci, r->left);

    printf("\nMon: %s - %s\n", r->data.mamh, r->data.tenmh);
    printf("%-12s | %-20s | %-4s | %s\n", "MASV", "HO TEN", "PHAI", "DIEM");
    for (int i = 0; i < ds.n; ++i) {
        if (su_stricmp(ds.nodes[i]->malop, malop_ci) != 0) continue;
        for (PTRSV p = ds.nodes[i]->FirstSV; p; p = p->next) {
            char hoten[80];
            std::snprintf(hoten, sizeof(hoten), "%s %s", p->data.ho, p->data.ten);
            printf("%-12s | %-20s | %-4s | ", p->data.masv, hoten, p->data.phai);
            print_diem_of_sv_for_mon(p, r->data.mamh);
            printf("\n");
        }
    }

    inorder_print_bangdiem(ds, malop_ci, r->right);
}

void in_bang_diem_lop(const DS_Lop& ds, const char* malop_ci, PTRMH root) {
    const Lop* lp = NULL;
    for (int i = 0; i < ds.n; ++i) {
        if (su_stricmp(ds.nodes[i]->malop, malop_ci) == 0) {
            lp = ds.nodes[i];
            break;
        }
    }

    if (!lp) {
        printf("Khong tim thay lop %s\n", malop_ci);
        return;
    }
    printf("=== BANG DIEM LOP %s - %s ===\n", lp->malop, lp->tenlop);
    inorder_print_bangdiem(ds, malop_ci, root);
}
