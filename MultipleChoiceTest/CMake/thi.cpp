// thi.cpp
// (thi tiep tuc khi bi vang + thanh trang thai dap an + canh bao thi lai + danh dau cau)
//
// Do phuc tap chinh (big-O):
// - find_sv_global:    O(tong_so_SV_toan_he_thong)
// - thuc_hien_thi:     O(soCauExam) cho viec cham diem + O(soCauExam) cho load/save/tron de
// - in_bang_diem_lop:  O(so_mon * so_SV_trong_lop)

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
#include <conio.h>

// ================== HANG SO GIAO DIEN ==================
static const int ROW_INFO      = 2;   // dong thong tin cau + thoi gian
static const int ROW_QUESTION  = 4;   // bat dau in cau hoi
static const int ROW_ANSWERBAR = 14;  // thanh trang thai cau / dap an

// ================== HELPERS CHUNG ==================

// lam tron 1 chu so thap phan (vd 7.26 -> 7.3)
// Do phuc tap: O(1)
static float round1(float x) {
    if (x >= 0) return (float)((int)(x * 10 + 0.5f)) / 10.0f;
    return (float)((int)(x * 10 - 0.5f)) / 10.0f;
}

// tim SV theo MASV (da normalize) tren toan he thong
// Do phuc tap: O(tong_so_SV_toan_he_thong)
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
// Do phuc tap: O(so_cau_cua_mon)
static int collect_questions(PTRCH head, PTRCH* arr, int cap) {
    int c = 0;
    for (PTRCH p = head; p && c < cap; p = p->next) arr[c++] = p;
    return c;
}

// tron ngau nhien mang chi so (Fisher-Yates)
// Do phuc tap: O(n)
static void shuffle_indices(int* idx, int n) {
    rng_seed_once();
    for (int i = n - 1; i > 0; --i) {
        int j = rand_in(0, i);
        int t = idx[i]; idx[i] = idx[j]; idx[j] = t;
    }
}

// ve thanh trang thai dap an + danh dau + mui ^ chi cau hien tai
// Do phuc tap: O(soCau)
static void draw_answer_bar(const ExamQItem* items,
                            const bool* marked,
                            int soCau,
                            int curIdx)
{
    // Dong so cau
    gotoxy(0, ROW_ANSWERBAR);
    printf("Cau : ");
    for (int i = 0; i < soCau; ++i) {
        printf("%2d ", i + 1);
    }
    printf("   ");

    // Dong dap an
    gotoxy(0, ROW_ANSWERBAR + 1);
    printf("Dap : ");
    for (int i = 0; i < soCau; ++i) {
        char c = items[i].da_chon;
        if (c != 'A' && c != 'B' && c != 'C' && c != 'D') c = '.';
        printf(" %c ", c);
    }
    printf("   ");

    // Dong danh dau
    gotoxy(0, ROW_ANSWERBAR + 2);
    printf("Mark: ");
    for (int i = 0; i < soCau; ++i) {
        char m = (marked && marked[i]) ? '*' : ' ';
        printf(" %c ", m);
    }
    printf("   ");

    // Dong mui ten chi cau hien tai
    gotoxy(0, ROW_ANSWERBAR + 3);
    printf("      ");
    for (int i = 0; i < soCau; ++i) {
        if (i == curIdx) printf(" ^ ");
        else             printf("   ");
    }
    printf("   ");
}

// ================== LUU / TAI BAI THI DANG LAM ==================
//
// File: InProgress_<MASV>_<MAMH>.txt
// Dong 1: soCau remainSec curIdx
// Dong 2..: id da_chon markFlag
//   - id       : id cau hoi
//   - da_chon  : 'A'/'B'/'C'/'D' hoac '?' neu chua chon
//   - markFlag : 0 hoac 1 (cau nay co dang duoc danh dau khong)
//

// Do phuc tap: O(1)
static void build_inprogress_filename(const char* masv_ci,
                                      const char* mamh_ci,
                                      char* out, int outSize) {
    if (!out || outSize <= 0) return;
    std::snprintf(out, outSize, "InProgress_%s_%s.txt", masv_ci, mamh_ci);
}

// Do phuc tap: O(soCau)
static bool save_inprogress_exam(const char* masv_ci, const char* mamh_ci,
                                 const ExamQItem* items,
                                 const bool* marked,
                                 int soCau,
                                 int curIdx,
                                 int remainSec)
{
    if (!items || soCau <= 0) return false;

    char path[256];
    build_inprogress_filename(masv_ci, mamh_ci, path, sizeof(path));

    FILE* f = std::fopen(path, "wt");
    if (!f) return false;

    std::fprintf(f, "%d %d %d\n", soCau, remainSec, curIdx);
    for (int i = 0; i < soCau; ++i) {
        char c = items[i].da_chon;
        if (!(c == 'A' || c == 'B' || c == 'C' || c == 'D')) c = '?';
        int markFlag = (marked && marked[i]) ? 1 : 0;
        std::fprintf(f, "%d %c %d\n", items[i].id, c, markFlag);
    }

    std::fclose(f);
    return true;
}

// Tai full bai thi dang lam: reconstruct ExamQItem & mang marked tu cay cau hoi
// Do phuc tap: O(soCau * soCauCuaMon) trong truong hop xau nhat (duyet list cau hoi de tim id)
static bool load_inprogress_exam(const char* masv_ci, const char* mamh_ci,
                                 PTRMH mh,
                                 ExamQItem*& items_out,
                                 bool*& marked_out,
                                 int& soCau,
                                 int& curIdx,
                                 int& remainSec)
{
    items_out  = NULL;
    marked_out = NULL;
    soCau = 0; curIdx = 0; remainSec = 0;

    char path[256];
    build_inprogress_filename(masv_ci, mamh_ci, path, sizeof(path));

    FILE* f = std::fopen(path, "rt");
    if (!f) return false;

    if (std::fscanf(f, "%d %d %d", &soCau, &remainSec, &curIdx) != 3) {
        std::fclose(f);
        return false;
    }

    if (soCau <= 0 || remainSec < 0 || curIdx < 0 || curIdx >= soCau) {
        std::fclose(f);
        return false;
    }

    int*  ids  = new int[soCau];
    char* ans  = new char[soCau];
    int*  mark = new int[soCau];

    for (int i = 0; i < soCau; ++i) {
        int id;
        char c;
        int markFlag = 0;
        int nread = std::fscanf(f, "%d %c %d", &id, &c, &markFlag);
        if (nread < 2) {
            delete[] ids;
            delete[] ans;
            delete[] mark;
            std::fclose(f);
            return false;
        }
        if (nread < 3) markFlag = 0; // ho tro file cu chi co 2 truong id + da_chon
        ids[i]  = id;
        ans[i]  = c;
        mark[i] = markFlag;
    }
    std::fclose(f);

    items_out  = new ExamQItem[soCau];
    marked_out = new bool[soCau];

    for (int i = 0; i < soCau; ++i) {
        int id = ids[i];
        PTRCH p = mh->data.FirstCHT;
        while (p && p->data.id != id) p = p->next;
        if (!p) {
            delete[] ids;
            delete[] ans;
            delete[] mark;
            delete[] items_out;
            delete[] marked_out;
            items_out  = NULL;
            marked_out = NULL;
            return false;
        }

        items_out[i].id = p->data.id;
        su_strncpy(items_out[i].mamh, mh->data.mamh, 16);
        su_strncpy(items_out[i].noidung, p->data.noidung, 501);
        su_strncpy(items_out[i].A, p->data.A, 201);
        su_strncpy(items_out[i].B, p->data.B, 201);
        su_strncpy(items_out[i].C, p->data.C, 201);
        su_strncpy(items_out[i].D, p->data.D, 201);
        items_out[i].dapan = p->data.dapan;

        char c = ans[i];
        if (!(c == 'A' || c == 'B' || c == 'C' || c == 'D')) c = '?';
        items_out[i].da_chon = c;

        marked_out[i] = (mark[i] != 0);
    }

    delete[] ids;
    delete[] ans;
    delete[] mark;
    return true;
}

// Do phuc tap: O(1) voi he dieu hanh Windows
static void delete_inprogress_exam(const char* masv_ci, const char* mamh_ci) {
    char path[256];
    build_inprogress_filename(masv_ci, mamh_ci, path, sizeof(path));
    std::remove(path);
}

// ================== THUC HIEN THI TRAC NGHIEM ==================
//
// Logic:
// - KHONG phu thuoc vao soCau / thoiGian tu menu de quyet dinh tiep tuc hay khong.
// - Neu co file InProgress_<MASV>_<MAMH>.txt:
//     + Hien thong tin -> hoi "co tiep tuc khong?"
//     + YES: tiep tuc dung de cu (khong hoi so cau / so phut).
//     + NO : xoa file tam, sau do dung tham so soCau, thoiGianPhut lam de moi.
// - Co chuc nang danh dau cau hoi bang phim 'M'.
//
int thuc_hien_thi(DS_Lop& ds, PTRMH root, PTRExamLog& logs,
                  const char* masv_ci, const char* mamh_ci,
                  int soCau, int thoiGianPhut)
{
    // --- Tim SV va Lop ---
    Lop* lop = NULL;
    PTRSV sv = find_sv_global(ds, masv_ci, &lop);
    if (!sv) {
        printf("Khong tim thay sinh vien.\n");
        return 1;
    }

    // --- Tim mon hoc ---
    PTRMH mh = find_monhoc(root, mamh_ci);
    if (!mh) {
        printf("Khong tim thay mon hoc.\n");
        return 2;
    }

    int n_q = dem_cau(mh->data.FirstCHT);
    if (n_q <= 0) {
        printf("Mon hoc nay chua co cau hoi nao, khong the thi.\n");
        return 4;
    }

    // --- Thu tai bai thi dang do (neu co) ---
    ExamQItem* items  = NULL;
    bool*      marked = NULL;
    int soCauExam     = 0;
    int curIdx        = 0;
    int remainSec     = 0;

    bool hasSaved = load_inprogress_exam(masv_ci, mamh_ci, mh,
                                         items, marked,
                                         soCauExam, curIdx, remainSec);

    if (hasSaved && remainSec > 0 && items && marked) {
        int soDaLam = 0;
        for (int i = 0; i < soCauExam; ++i) {
            char c = items[i].da_chon;
            if (c == 'A' || c == 'B' || c == 'C' || c == 'D') ++soDaLam;
        }

        printf("\nBAN DANG CO MOT BAI THI CHUA HOAN THANH CHO MON %s.\n", mh->data.mamh);
        printf("- So cau: %d (da tra loi: %d)\n", soCauExam, soDaLam);
        printf("- Thoi gian con lai (khong tinh thoi gian ban thoat chuong trinh): %02d:%02d\n",
               remainSec / 60, remainSec % 60);

        if (confirm_dialog("Ban co muon TIEP TUC bai thi nay khong")) {
            // tiep tuc -> bo qua canh bao thi lai (vi luc bat dau da canh bao)
        } else {
            // huy bai cu -> xoa file tam + giai phong items/marked
            delete_inprogress_exam(masv_ci, mamh_ci);
            delete[] items;
            delete[] marked;
            items      = NULL;
            marked     = NULL;
            soCauExam  = 0;
            curIdx     = 0;
            remainSec  = 0;
            hasSaved   = false;
        }
    }

    // --- Neu khong tiep tuc bai dang do -> THI MOI / THI LAI ---
    if (!hasSaved || !items || !marked) {
        // Canh bao THI LAI neu da co bai thi trong log
        ExamRecord* oldExam = find_exam(logs, masv_ci, mamh_ci);
        if (oldExam) {
            printf("\nBAN DA TUNG THI MON NAY TRUOC DO.\n");
            printf("- Diem hien tai: %.1f\n", oldExam->diem);
            printf("- Thi lai se GHI DE diem va bai thi cu.\n");
            if (!confirm_dialog("Ban co chac muon THI LAI mon nay khong")) {
                printf("Da huy thi mon nay.\n");
                return 0; // khong phai loi, chi huy
            }
        }

        int soCauUse  = soCau;
        int soPhutUse = thoiGianPhut;

        // Neu menu truyen soCau/thoiGian hop le thi dung luon, khong hoi lai.
        if (soCauUse <= 0 || soCauUse > n_q) {
            printf("\nMon %s - %s dang co %d cau hoi.\n",
                   mh->data.mamh, mh->data.tenmh, n_q);
            printf("Ban chi duoc chon so cau thi tu 1 den %d.\n", n_q);

            char buf[64];
            printf("So cau muon thi: ");
            std::fgets(buf, sizeof(buf), stdin); chomp_line(buf);
            soCauUse = std::atoi(buf);
        }

        if (soCauUse <= 0) {
            printf("So cau thi khong hop le (phai > 0).\n");
            return 3;
        }
        if (soCauUse > n_q) {
            printf("Mon nay chi co %d cau hoi. Vui long nhap so cau <= %d.\n", n_q, n_q);
            return 4;
        }

        if (soPhutUse <= 0) {
            char buf[64];
            printf("So phut lam bai: ");
            std::fgets(buf, sizeof(buf), stdin); chomp_line(buf);
            soPhutUse = std::atoi(buf);
        }
        if (soPhutUse <= 0) {
            printf("So phut lam bai phai > 0.\n");
            return 3;
        }

        // Random de moi
        PTRCH* arr = new PTRCH[n_q];
        int cnt    = collect_questions(mh->data.FirstCHT, arr, n_q);

        int* idxArr = new int[cnt];
        for (int i = 0; i < cnt; ++i) idxArr[i] = i;
        shuffle_indices(idxArr, cnt);

        items  = new ExamQItem[soCauUse];
        marked = new bool[soCauUse];

        for (int i = 0; i < soCauUse; ++i) {
            PTRCH q = arr[idxArr[i]];
            items[i].id = q->data.id;
            su_strncpy(items[i].mamh, mh->data.mamh, 16);
            su_strncpy(items[i].noidung, q->data.noidung, 501);
            su_strncpy(items[i].A, q->data.A, 201);
            su_strncpy(items[i].B, q->data.B, 201);
            su_strncpy(items[i].C, q->data.C, 201);
            su_strncpy(items[i].D, q->data.D, 201);
            items[i].dapan   = q->data.dapan;
            items[i].da_chon = '?';
            marked[i]        = false;
        }

        delete[] arr;
        delete[] idxArr;

        soCauExam = soCauUse;
        curIdx    = 0;
        remainSec = soPhutUse * 60;

        // luu trang thai luc bat dau (phong tru program bi tat som)
        save_inprogress_exam(masv_ci, mamh_ci, items, marked, soCauExam, curIdx, remainSec);
    }

    // ================== CHAY VONG THI (dung cho ca tiep tuc & thi moi) ==================
    clearScreen();
    hideCursor();
    printf("THI TRAC NGHIEM - SV: %s  - Lop: %s  - Mon: %s\n",
           sv->data.masv,
           lop ? lop->malop : "(?)",
           mh->data.mamh);
    printf("So cau: %d   (M: danh dau, F9: NOP SOM, ESC: HOI NOP BAI)\n", soCauExam);
    printf("---------------------------------------------------------------------\n");

    // endTick = now + remainSec
    ULONGLONG endTick = GetTickCount64() + (ULONGLONG)remainSec * 1000ULL;
    int cur = curIdx;

    while (1) {
        int remain = (int)((endTick - GetTickCount64()) / 1000ULL);
        if (remain < 0) remain = 0;

        // Dong thong tin cau + thoi gian
        gotoxy(0, ROW_INFO);
        printf("Cau %d/%d   ", cur + 1, soCauExam);
        print_mmss_at(25, ROW_INFO, remain);

        // In noi dung cau hoi
        gotoxy(0, ROW_QUESTION);
        printf("Noi dung: %-70s\n", items[cur].noidung);
        printf("A) %-70s\n", items[cur].A);
        printf("B) %-70s\n", items[cur].B);
        printf("C) %-70s\n", items[cur].C);
        printf("D) %-70s\n", items[cur].D);
        printf("Chon (A/B/C/D), M: danh dau/bo danh dau, <- ->: lui/tien, F9: nop som, ESC: hoi nop bai.\n");

        if (items[cur].da_chon == '?')
            printf("Da chon: (chua chon)   ");
        else
            printf("Da chon: %c            ", items[cur].da_chon);
        printf("Trang thai danh dau: %s\n", marked[cur] ? "CO" : "KHONG");

        // Thanh trang thai dap an + danh dau
        draw_answer_bar(items, marked, soCauExam, cur);

        if (remain == 0) {
            gotoxy(0, ROW_QUESTION + 9);
            printf("Het gio! Tu dong nop bai...\n");
            Sleep(1000);
            break;
        }

        int key = readKey();
        if (key == KEY_NONE) { Sleep(60); continue; }

        bool changed = false;

        if (key == KEY_ESC) {
            if (confirm_dialog("Ban co muon NOP BAI ngay bay gio khong")) {
                save_inprogress_exam(masv_ci, mamh_ci, items, marked, soCauExam, cur, remain);
                break;
            }
        } else if (key == KEY_F9) {
            if (confirm_dialog("Ban co muon NOP SOM khong")) {
                save_inprogress_exam(masv_ci, mamh_ci, items, marked, soCauExam, cur, remain);
                break;
            }
        } else if (key == KEY_LEFT) {
            if (cur > 0) { --cur; changed = true; }
        } else if (key == KEY_RIGHT || key == KEY_ENTER) {
            if (cur < soCauExam - 1) { ++cur; changed = true; }
        } else {
            if (key >= 'a' && key <= 'z') key = key - 'a' + 'A';

            if (key == 'A' || key == 'B' || key == 'C' || key == 'D') {
                if (items[cur].da_chon != (char)key) {
                    items[cur].da_chon = (char)key;
                    changed = true;
                }
                if (cur < soCauExam - 1) { ++cur; changed = true; }
            } else if (key == 'M') {
                // Toggle danh dau cau hien tai
                marked[cur] = !marked[cur];
                changed = true;
            }
        }

        if (changed) {
            // luu trang thai moi nhat (dap an + cau hien tai + thoi gian con lai + danh dau)
            save_inprogress_exam(masv_ci, mamh_ci, items, marked, soCauExam, cur, remain);
        }
    }

    showCursor();

    // Bai thi da ket thuc -> xoa file tam
    delete_inprogress_exam(masv_ci, mamh_ci);

    // Tinh diem
    int dung = 0;
    for (int i = 0; i < soCauExam; ++i) {
        if (items[i].da_chon == items[i].dapan) ++dung;
    }
    float diem = round1(10.0f * dung / (float)soCauExam);

    // Luu bai thi + diem
    upsert_exam(logs, sv->data.masv, mh->data.mamh, items, soCauExam, dung, diem);
    upsert_diem(sv->data.ds_diemthi, mh->data.mamh, diem);

    printf("\nDa NOP BAI.\n");
    printf("- So cau dung: %d/%d\n", dung, soCauExam);
    printf("- Diem: %.1f\n", diem);
    printf("Ket qua da duoc LUu.\n");
    printf("Ban co the xem lai tai menu:\n");
    printf("- 'Xem diem cac mon'\n");
    printf("- 'Xem chi tiet bai thi 1 mon'\n");
    printf("\nNhan phim bat ky de tiep tuc...");
    _getch();

    delete[] marked;
    delete[] items;
    return 0;
}

// ================== IN CHI TIET BAI THI CUA 1 SV CHO 1 MON ==================

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

// ================== IN BANG DIEM 1 LOP ==================

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

// Do phuc tap: O(so_mon * so_SV_trong_lop)
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
