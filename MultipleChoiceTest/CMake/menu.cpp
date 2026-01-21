// menu.cpp (da toi uu: UX + AUTO-SAVE + UNDO + SORT + validate input + chan nhap toan space)
// + NEW: Global Auto-Increment Question ID (O(1) counter) NHUNG ID xuat ra "nhin ngau nhien" bang hoan vi 1-1 (khong trung, khong check tung lan)
// + NEW: Chan dap an trung nhau (2/3/4 dap an trung deu KHONG cho luu)

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
#include "diemthi.h"

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <windows.h> // Sleep

// --------------------------------------------------
// Duong dan file TXT chinh
// --------------------------------------------------
static const char* PATH_LOP   = "Lop.txt";
static const char* PATH_SV    = "SinhVien.txt";
static const char* PATH_MON   = "MonHoc.txt";
static const char* PATH_CH    = "CauHoi.txt";
static const char* PATH_SVDT  = "SinhVienDaThi.txt";

// NEW: meta ID cau hoi (de ID khong bao gio bi "lui" khi xoa cau hoi)
static const char* PATH_QID      = "QuestionId.meta";

// --------------------------------------------------
// Duong dan file snapshot (UNDO)
// --------------------------------------------------
static const char* PATH_LOP_BAK   = "Lop.bak";
static const char* PATH_SV_BAK    = "SinhVien.bak";
static const char* PATH_MON_BAK   = "MonHoc.bak";
static const char* PATH_CH_BAK    = "CauHoi.bak";
static const char* PATH_SVDT_BAK  = "SinhVienDaThi.bak";

// NEW: snapshot meta ID
static const char* PATH_QID_BAK   = "QuestionId.bak";

// --------------------------------------------------
// Bien trang thai toan cuc
// --------------------------------------------------
static DS_Lop     g_dslop;
static PTRMH      g_rootMH        = NULL;
static PTRExamLog g_logs          = NULL;
static bool       g_exit_app      = false;
static bool       g_has_undo_snap = false;   // da co snapshot cho Undo chua

// NEW: next question counter (global auto-increment O(1))
static int        g_next_qid      = 1;

// Mo ta lan thao tac gan nhat (de show khi Undo)
static char g_last_op_title[64]   = "";
static char g_last_op_detail[128] = "";

// ================================================================
// ================== ID CAU HOI (COUNTER + HOAN VI 1-1) ===========
// ================================================================
//
// Yeu cau: ID "nhin ngau nhien", KHONG check trung tung lan, O(1) cap phat.
// Giai phap: tao ID = encode(counter) voi encode la phep hoan vi 1-1 tren 1 tap so.
// -> counter tang dan => ID khong bao gio trung.
// -> decode(ID) cho ra counter de quet phuc hoi g_next_qid neu mat meta.
//
// De tranh va cham voi du lieu cu (neu truoc day id=1,2,3...):
// reserve 1..1,000,000 cho legacy. ID moi luon > reserve.
//
// ID moi nam trong [RESERVE+1 .. 2147483646] (fit int).
//

static const int64_t QID_PRIME   = 2147483647LL;   // 2^31-1 (prime)
static const int64_t QID_RESERVE = 1000000LL;      // giu vung id nho cho legacy
static const int64_t QID_RANGE   = QID_PRIME - QID_RESERVE - 1; // so phan tu vung id moi

// Linear congruential permutation parameters (can gcd(A, QID_RANGE)=1)
static const int64_t QID_A = 1103515245LL;
static const int64_t QID_B = 12345LL;

// normalize modulo (luon ra [0..m-1])
static int64_t mod_norm(int64_t x, int64_t m) {
    x %= m;
    if (x < 0) x += m;
    return x;
}

// extended gcd: return gcd(a,b), and outputs x,y such that ax+by=g
static int64_t egcd(int64_t a, int64_t b, int64_t &x, int64_t &y) {
    if (b == 0) {
        x = 1; y = 0;
        return a;
    }
    int64_t x1=0, y1=0;
    int64_t g = egcd(b, a % b, x1, y1);
    x = y1;
    y = x1 - (a / b) * y1;
    return g;
}

// modular inverse: a^-1 mod m (assumes gcd(a,m)=1)
static int64_t modinv(int64_t a, int64_t m) {
    int64_t x=0, y=0;
    int64_t g = egcd(a, m, x, y);
    if (g != 1 && g != -1) return -1;
    x %= m;
    if (x < 0) x += m;
    return x;
}

static int64_t QID_A_INV = -1;

// encode counter -> id
// counter in [1..QID_RANGE]  (we map counter-1 in [0..QID_RANGE-1])
static int qid_encode(int counter) {
    if (counter < 1) counter = 1;
    if ((int64_t)counter > QID_RANGE) counter = (int)QID_RANGE;

    int64_t c = (int64_t)counter - 1; // 0..range-1
    int64_t x = (QID_A * c + QID_B) % QID_RANGE; // 0..range-1
    int64_t id = (QID_RESERVE + 1) + x;          // reserve+1 .. reserve+range
    if (id < 1) id = 1;
    if (id > 2147483646LL) id = 2147483646LL;
    return (int)id;
}

// decode id -> counter
// if id <= reserve: legacy, return 0 (khong tinh vao counter moi)
// else counter = (invA * (x - B) mod range) + 1
static int qid_decode_to_counter(int id) {
    if (id <= (int)QID_RESERVE) return 0; // legacy range
    int64_t x = (int64_t)id - (QID_RESERVE + 1); // 0..range-1 (neu hop le)
    x = mod_norm(x, QID_RANGE);

    int64_t t = mod_norm(x - QID_B, QID_RANGE);
    if (QID_A_INV < 0) return 0;

    int64_t c = (QID_A_INV * t) % QID_RANGE; // 0..range-1
    int64_t counter = c + 1;                 // 1..range
    if (counter < 1) counter = 1;
    if (counter > QID_RANGE) counter = QID_RANGE;
    return (int)counter;
}

// init inverse (call once)
static void qid_init_once() {
    if (QID_A_INV >= 0) return;
    QID_A_INV = modinv(QID_A, QID_RANGE);
    if (QID_A_INV < 0) {
        // fallback an toan (khong nen xay ra vi gcd(A,range)=1)
        QID_A_INV = 1;
    }
}

// quet danh sach cau hoi 1 mon -> tim max counter cua scheme moi
static int max_counter_in_question_list(PTRCH first) {
    int mx = 0;
    for (PTRCH p = first; p; p = p->next) {
        int c = qid_decode_to_counter(p->data.id);
        if (c > mx) mx = c;
    }
    return mx;
}

// quet tat ca mon (AVL) -> tim max counter cua scheme moi
static int max_counter_in_all_subjects(PTRMH root) {
    if (!root) return 0;
    int mxL = max_counter_in_all_subjects(root->left);
    int mxR = max_counter_in_all_subjects(root->right);
    int mxM = max_counter_in_question_list(root->data.FirstCHT);
    int mx = mxL;
    if (mxM > mx) mx = mxM;
    if (mxR > mx) mx = mxR;
    return mx;
}

// Meta file: ho tro 2 format
// 1) cu: "<nextCounter>"
// 2) moi: "v2 <nextCounter>"
static bool load_qid_meta(const char* path) {
    FILE* f = std::fopen(path, "r");
    if (!f) return false;

    char tag[16] = {0};
    long long v = 0;

    int c = std::fscanf(f, "%15s", tag);
    if (c != 1) { std::fclose(f); return false; }

    if (tag[0] == 'v' || tag[0] == 'V') {
        // format moi: v2 <num>
        int ok = std::fscanf(f, "%lld", &v);
        std::fclose(f);
        if (ok != 1) return false;
    } else {
        // format cu: tag chinh la so
        v = std::atoll(tag);
        std::fclose(f);
    }

    if (v < 1) v = 1;
    if (v > QID_RANGE) v = QID_RANGE;

    g_next_qid = (int)v;
    return true;
}

static bool save_qid_meta(const char* path) {
    FILE* f = std::fopen(path, "w");
    if (!f) return false;
    // ghi format moi de ro rang scheme
    std::fprintf(f, "v2 %d\n", g_next_qid);
    std::fclose(f);
    return true;
}

// Neu meta bi mat/hong -> quet max 1 lan luc load (O(tong so cau hoi) 1 lan)
static void sync_qid_from_data_if_needed() {
    int mxCounter = max_counter_in_all_subjects(g_rootMH);
    if (mxCounter < 0) mxCounter = 0;

    // next counter phai > max counter da dung
    if (g_next_qid <= mxCounter) g_next_qid = mxCounter + 1;

    if (g_next_qid < 1) g_next_qid = 1;
    if ((int64_t)g_next_qid > QID_RANGE) g_next_qid = (int)QID_RANGE;
}

// Phat ID O(1), KHONG check tung lan.
// return: ID "nhin ngau nhien" (encode(counter))
static int new_question_id_auto() {
    qid_init_once();

    int counter = g_next_qid;
    if (counter < 1) counter = 1;
    if ((int64_t)counter > QID_RANGE) counter = (int)QID_RANGE;

    int id = qid_encode(counter);

    // tang counter (khong wrap de tranh trung)
    if ((int64_t)g_next_qid < QID_RANGE) g_next_qid++;
    else g_next_qid = (int)QID_RANGE;

    return id;
}

// ================== CHAN DAP AN TRUNG NHAU ==================

static void trim_local(char* s) {
    if (!s) return;
    int n = (int)std::strlen(s);
    int l = 0;
    while (l < n && (s[l] == ' ' || s[l] == '\t' || s[l] == '\r' || s[l] == '\n')) l++;
    int r = n - 1;
    while (r >= l && (s[r] == ' ' || s[r] == '\t' || s[r] == '\r' || s[r] == '\n')) r--;
    int k = 0;
    for (int i = l; i <= r; ++i) s[k++] = s[i];
    s[k] = '\0';
}

static void squeeze_spaces_local(char* s) {
    if (!s) return;
    int n = (int)std::strlen(s);
    int k = 0;
    bool in_space = false;
    for (int i = 0; i < n; ++i) {
        char c = s[i];
        bool isSpace = (c == ' ' || c == '\t');
        if (isSpace) {
            if (!in_space) {
                s[k++] = ' ';
                in_space = true;
            }
        } else {
            s[k++] = c;
            in_space = false;
        }
    }
    s[k] = '\0';
}

static bool is_blank_str(const char* s) {
    if (!s) return true;
    for (const char* p = s; *p; ++p) {
        if (*p != ' ' && *p != '\t') return false;
    }
    return true;
}

static void normalize_answer(char* s) {
    if (!s) return;
    trim_local(s);
    squeeze_spaces_local(s);
    // sau normalize ma rong -> coi nhu blank
}

static bool answers_all_distinct(char* A, char* B, char* C, char* D) {
    normalize_answer(A);
    normalize_answer(B);
    normalize_answer(C);
    normalize_answer(D);

    if (is_blank_str(A) || is_blank_str(B) || is_blank_str(C) || is_blank_str(D)) return false;

    if (su_stricmp(A, B) == 0) return false;
    if (su_stricmp(A, C) == 0) return false;
    if (su_stricmp(A, D) == 0) return false;
    if (su_stricmp(B, C) == 0) return false;
    if (su_stricmp(B, D) == 0) return false;
    if (su_stricmp(C, D) == 0) return false;
    return true;
}

// ================== Luu / nap toan bo ==================

static void save_all() {
    if (!ghi_lop_txt(PATH_LOP, g_dslop))            printf("Loi ghi %s\n", PATH_LOP);
    if (!ghi_sinhvien_txt(PATH_SV, g_dslop))        printf("Loi ghi %s\n", PATH_SV);
    if (!ghi_monhoc_txt(PATH_MON, g_rootMH))        printf("Loi ghi %s\n", PATH_MON);
    if (!ghi_cauhoi_txt(PATH_CH, g_rootMH))         printf("Loi ghi %s\n", PATH_CH);
    if (!ghi_sinhvien_dathi_txt(PATH_SVDT, g_logs)) printf("Loi ghi %s\n", PATH_SVDT);

    // NEW: luu next question counter (scheme v2)
    if (!save_qid_meta(PATH_QID))                   printf("Loi ghi %s\n", PATH_QID);

    printf("Da luu du lieu.\n");
}

static void load_all() {
    qid_init_once();

    free_ds_lop(g_dslop);
    free_all_monhoc(g_rootMH);
    free_examlog(g_logs);

    // Thu tu nap: Lop -> SV -> Mon -> Cau hoi -> Bai thi
    doc_lop_txt(PATH_LOP, g_dslop);
    doc_sinhvien_txt(PATH_SV, g_dslop);
    doc_monhoc_txt(PATH_MON, g_rootMH);
    doc_cauhoi_txt(PATH_CH, g_rootMH);

    // NEW: load meta next counter, neu khong co thi quet max counter 1 lan
    if (!load_qid_meta(PATH_QID)) {
        g_next_qid = max_counter_in_all_subjects(g_rootMH) + 1;
        if (g_next_qid < 1) g_next_qid = 1;
        if ((int64_t)g_next_qid > QID_RANGE) g_next_qid = (int)QID_RANGE;
        save_qid_meta(PATH_QID);
    } else {
        sync_qid_from_data_if_needed();
    }

    doc_sinhvien_dathi_txt(PATH_SVDT, g_logs, g_dslop);
}

// ================== UNDO ==================

static void set_last_op(const char* title, const char* detail) {
    if (!title)  title  = "";
    if (!detail) detail = "";
    std::snprintf(g_last_op_title,  sizeof(g_last_op_title),  "%s", title);
    std::snprintf(g_last_op_detail, sizeof(g_last_op_detail), "%s", detail);
}

// Chi luu snapshot, khong hoi nguoi dung
static bool save_undo_snapshot() {
    bool ok = true;
    ok &= ghi_lop_txt(PATH_LOP_BAK, g_dslop);
    ok &= ghi_sinhvien_txt(PATH_SV_BAK, g_dslop);
    ok &= ghi_monhoc_txt(PATH_MON_BAK, g_rootMH);
    ok &= ghi_cauhoi_txt(PATH_CH_BAK, g_rootMH);
    ok &= ghi_sinhvien_dathi_txt(PATH_SVDT_BAK, g_logs);

    // NEW: snapshot meta next counter (scheme v2)
    ok &= save_qid_meta(PATH_QID_BAK);

    if (!ok) {
        printf("Canh bao: Khong tao duoc du lieu hoan tac.\n");
        printf("Lenh Undo co the khong dung duoc cho lan thay doi nay.\n");
    } else {
        g_has_undo_snap = true;
    }
    return ok;
}

// Thuc hien revert ve snapshot (khong hoi nguoi dung)
static bool undo_last_change_raw() {
    if (!g_has_undo_snap) {
        return false;
    }

    qid_init_once();

    // Xoa du lieu hien tai trong RAM
    free_ds_lop(g_dslop);
    free_all_monhoc(g_rootMH);
    free_examlog(g_logs);

    bool ok = true;
    ok &= doc_lop_txt(PATH_LOP_BAK, g_dslop);
    ok &= doc_sinhvien_txt(PATH_SV_BAK, g_dslop);
    ok &= doc_monhoc_txt(PATH_MON_BAK, g_rootMH);
    ok &= doc_cauhoi_txt(PATH_CH_BAK, g_rootMH);

    // NEW: restore next counter (neu bak khong co thi quet max 1 lan)
    if (!load_qid_meta(PATH_QID_BAK)) {
        g_next_qid = max_counter_in_all_subjects(g_rootMH) + 1;
        if (g_next_qid < 1) g_next_qid = 1;
        if ((int64_t)g_next_qid > QID_RANGE) g_next_qid = (int)QID_RANGE;
        save_qid_meta(PATH_QID_BAK);
    } else {
        sync_qid_from_data_if_needed();
    }

    ok &= doc_sinhvien_dathi_txt(PATH_SVDT_BAK, g_logs, g_dslop);

    if (ok) {
        g_has_undo_snap = false;
        g_last_op_title[0]   = '\0';
        g_last_op_detail[0]  = '\0';
    }
    return ok;
}

// Ham goi tu menu: show thong tin + confirm + goi undo_raw
static void menu_undo_action() {
    if (!g_has_undo_snap) {
        printf("\nKhong co thay doi nao de hoan tac.\n");
        system("pause");
        return;
    }

    printf("\n=== HOAN TAC THAY DOI GAN NHAT (UNDO TOAN HE THONG) ===\n");
    printf("Lenh nay se dua tat ca du lieu Lop, Sinh vien, Mon hoc,\n");
    printf("Cau hoi va Bai thi ve trang thai truoc lan thay doi gan nhat.\n\n");
    printf("%-10s | %s\n", "THAO TAC",
           (g_last_op_title[0] != '\0') ? g_last_op_title : "(khong ro)");
    printf("%-10s | %s\n", "CHI TIET",
           (g_last_op_detail[0] != '\0') ? g_last_op_detail : "(khong ro)");
    printf("--------------------------------------------------------\n");

    if (!confirm_dialog("Ban co chac muon HOAN TAC ve trang thai nay")) {
        printf("Da huy hoan tac.\n");
        system("pause");
        return;
    }

    if (undo_last_change_raw()) {
        printf("Hoan tac thanh cong.\n");
        save_all();
    } else {
        printf("Hoan tac that bai (co the file *.bak da bi xoa/hong).\n");
    }
    system("pause");
}

// ================== Helpers chung ==================

// Doc 1 phim tu readKey() (non-blocking) cho den khi co phim
static int wait_key() {
    int k;
    do {
        k = readKey();
        if (k == KEY_NONE) Sleep(50);
    } while (k == KEY_NONE);
    return k;
}

// Helper doc so nguyen tu stdin, co thong bao loi chung
static bool read_int(const char* prompt, int& out) {
    printf("%s", prompt);
    if (std::scanf("%d", &out) != 1) {
        while (getchar() != '\n');
        printf("Nhap khong hop le (khong phai so nguyen).\n");
        system("pause");
        return false;
    }
    while (getchar() != '\n'); // an newline
    return true;
}

// Xu ly ESC o moi menu: tu dong luu + thoat chuong trinh
static bool handle_esc_exit() {
    printf("\nDang thoat va luu du lieu...\n");
    save_all();
    g_exit_app = true;
    return true;
}

// ================== Helpers: kiem tra / sort LOP ==================

static int cmp_lop_by_code(const Lop* a, const Lop* b) {
    return su_stricmp(a->malop, b->malop);
}

static void swap_lop_ptr(Lop*& a, Lop*& b) {
    Lop* t = a;
    a = b;
    b = t;
}

static void quicksort_lop(Lop** arr, int left, int right) {
    if (left >= right) return;
    int i = left, j = right;
    Lop* pivot = arr[(left + right) / 2];

    while (i <= j) {
        while (cmp_lop_by_code(arr[i], pivot) < 0) ++i;
        while (cmp_lop_by_code(arr[j], pivot) > 0) --j;
        if (i <= j) {
            swap_lop_ptr(arr[i], arr[j]);
            ++i; --j;
        }
    }
    if (left < j)  quicksort_lop(arr, left, j);
    if (i < right) quicksort_lop(arr, i, right);
}

static int build_sorted_lop_array(const DS_Lop& ds, Lop**& arr) {
    arr = nullptr;
    if (ds.n <= 0) return 0;

    int n = ds.n;
    arr = new Lop*[n];
    for (int i = 0; i < n; ++i) {
        arr[i] = ds.nodes[i];
    }
    quicksort_lop(arr, 0, n - 1);
    return n;
}

static int find_lop_index(const DS_Lop& ds, Lop* target) {
    for (int i = 0; i < ds.n; ++i) {
        if (ds.nodes[i] == target) return i;
    }
    return -1;
}

// ================== Helpers: bang chon / in LOP ==================

static void print_ds_lop_overview() {
    if (g_dslop.n == 0) {
        printf("\nChua co lop nao. Hay vao menu 'Quan ly Lop' de them lop moi.\n");
        return;
    }

    Lop** arr = nullptr;
    int n = build_sorted_lop_array(g_dslop, arr);
    if (n <= 0) {
        printf("\nChua co lop nao.\n");
        return;
    }

    printf("\n=== DANH SACH LOP ===\n");
    printf("%-4s | %-12s | %-30s | %-4s\n", "STT", "MALOP", "TENLOP", "SV");
    printf("---------------------------------------------------------------\n");
    for (int i = 0; i < n; ++i) {
        const Lop* lp = arr[i];
        printf("%-4d | %-12s | %-30s | %-4d\n",
               i + 1, lp->malop, lp->tenlop, so_sinh_vien_trong_lop(lp));
    }

    delete[] arr;
}

static int chon_lop_index_tu_bang() {
    if (g_dslop.n == 0) {
        printf("\nChua co lop nao. Hay them lop truoc.\n");
        system("pause");
        return -1;
    }

    Lop** arr = nullptr;
    int n = build_sorted_lop_array(g_dslop, arr);
    if (n <= 0) {
        printf("\nChua co lop nao. Hay them lop truoc.\n");
        system("pause");
        return -1;
    }

    printf("\n=== DANH SACH LOP ===\n");
    printf("%-4s | %-12s | %-30s | %-4s\n", "STT", "MALOP", "TENLOP", "SV");
    printf("---------------------------------------------------------------\n");
    for (int i = 0; i < n; ++i) {
        const Lop* lp = arr[i];
        printf("%-4d | %-12s | %-30s | %-4d\n",
               i + 1, lp->malop, lp->tenlop, so_sinh_vien_trong_lop(lp));
    }

    int stt;
    if (!read_int("Nhap STT lop (0 = quay lai): ", stt)) {
        delete[] arr;
        return -1;
    }

    if (stt == 0) {
        delete[] arr;
        return -1;
    }
    if (stt < 1 || stt > n) {
        printf("STT khong hop le.\n");
        system("pause");
        delete[] arr;
        return -1;
    }

    Lop* chosen = arr[stt - 1];
    delete[] arr;

    int idx = find_lop_index(g_dslop, chosen);
    if (idx < 0) {
        printf("Loi noi bo: khong tim thay lop trong danh sach.\n");
        system("pause");
    }
    return idx;
}

// ================== Helpers: SORT sinh vien trong 1 lop ==================

static void get_last_word(const char* src, char* dst, int dstSize) {
    if (!src || dstSize <= 0) {
        if (dstSize > 0) dst[0] = '\0';
        return;
    }

    int len = (int)su_strlen(src);
    int end = len - 1;

    while (end >= 0 && src[end] == ' ') end--;
    if (end < 0) {
        dst[0] = '\0';
        return;
    }

    int start = end;
    while (start >= 0 && src[start] != ' ') start--;
    start++;

    int copyLen = end - start + 1;
    if (copyLen >= dstSize) copyLen = dstSize - 1;
    for (int i = 0; i < copyLen; ++i) {
        dst[i] = src[start + i];
    }
    dst[copyLen] = '\0';
}

static int cmp_sv_by_name(PTRSV a, PTRSV b) {
    char fullA[128], fullB[128];
    std::snprintf(fullA, sizeof(fullA), "%s %s", a->data.ho, a->data.ten);
    std::snprintf(fullB, sizeof(fullB), "%s %s", b->data.ho, b->data.ten);

    char lastA[64], lastB[64];
    get_last_word(fullA, lastA, sizeof(lastA));
    get_last_word(fullB, lastB, sizeof(lastB));

    int c = su_stricmp(lastA, lastB);
    if (c != 0) return c;

    c = su_stricmp(fullA, fullB);
    if (c != 0) return c;

    return su_stricmp(a->data.masv, b->data.masv);
}

static void swap_sv_ptr(PTRSV &a, PTRSV &b) {
    PTRSV t = a;
    a = b;
    b = t;
}

static void quicksort_sv(PTRSV *arr, int left, int right) {
    if (left >= right) return;
    int i = left, j = right;
    PTRSV pivot = arr[(left + right) / 2];

    while (i <= j) {
        while (cmp_sv_by_name(arr[i], pivot) < 0) ++i;
        while (cmp_sv_by_name(arr[j], pivot) > 0) --j;
        if (i <= j) {
            swap_sv_ptr(arr[i], arr[j]);
            ++i; --j;
        }
    }
    if (left < j)  quicksort_sv(arr, left, j);
    if (i < right) quicksort_sv(arr, i, right);
}

static int build_sorted_sv_array(Lop *lop, PTRSV *&arr) {
    arr = nullptr;
    if (!lop || !lop->FirstSV) return 0;

    int n = dem_sv(lop->FirstSV);
    if (n <= 0) return 0;

    arr = new PTRSV[n];
    int idx = 0;
    for (PTRSV p = lop->FirstSV; p; p = p->next) {
        arr[idx++] = p;
    }

    quicksort_sv(arr, 0, n - 1);
    return n;
}

static bool masv_exists_in_system(const char* masv_ci) {
    for (int i = 0; i < g_dslop.n; ++i) {
        Lop* lop = g_dslop.nodes[i];
        for (PTRSV p = lop->FirstSV; p; p = p->next) {
            if (su_stricmp(p->data.masv, masv_ci) == 0) return true;
        }
    }
    return false;
}

// ================== Helpers: bang chon SINH VIEN ==================

static PTRSV chon_sv_trong_lop(Lop* lop) {
    if (!lop || !lop->FirstSV) {
        printf("\nLop chua co sinh vien nao.\n");
        system("pause");
        return NULL;
    }

    PTRSV *arr = nullptr;
    int n = build_sorted_sv_array(lop, arr);
    if (n <= 0) {
        printf("\nLop chua co sinh vien nao.\n");
        system("pause");
        return NULL;
    }

    printf("\n=== DS SINH VIEN CUA LOP %s - %s ===\n", lop->malop, lop->tenlop);
    printf("%-4s | %-12s | %-20s | %-4s | %-8s\n",
           "STT", "MASV", "HO TEN", "PHAI", "Mon thi");
    printf("---------------------------------------------------------------------\n");

    for (int i = 0; i < n; ++i) {
        PTRSV p = arr[i];
        char hoten[80];
        std::snprintf(hoten, sizeof(hoten), "%s %s", p->data.ho, p->data.ten);
        printf("%-4d | %-12s | %-20s | %-4s | %-8d\n",
               i + 1,
               p->data.masv,
               hoten,
               p->data.phai,
               dem_diem(p->data.ds_diemthi));
    }

    int stt;
    if (!read_int("Nhap STT sinh vien (0 = quay lai): ", stt)) {
        delete[] arr;
        return NULL;
    }

    if (stt == 0) {
        delete[] arr;
        return NULL;
    }
    if (stt < 1 || stt > n) {
        printf("STT khong hop le.\n");
        system("pause");
        delete[] arr;
        return NULL;
    }

    PTRSV chosen = arr[stt - 1];
    delete[] arr;
    return chosen;
}

static void in_ds_sv_cua_lop(Lop* lop) {
    if (!lop || !lop->FirstSV) {
        printf("\nLop chua co sinh vien nao.\n");
        return;
    }

    PTRSV *arr = nullptr;
    int n = build_sorted_sv_array(lop, arr);
    if (n <= 0) {
        printf("\nLop chua co sinh vien nao.\n");
        return;
    }

    printf("\n=== DS SINH VIEN CUA LOP %s - %s ===\n", lop->malop, lop->tenlop);
    printf("%-4s | %-12s | %-20s | %-4s | %-8s\n",
           "STT", "MASV", "HO TEN", "PHAI", "Mon thi");
    printf("---------------------------------------------------------------------\n");

    for (int i = 0; i < n; ++i) {
        PTRSV p = arr[i];
        char hoten[80];
        std::snprintf(hoten, sizeof(hoten), "%s %s", p->data.ho, p->data.ten);
        printf("%-4d | %-12s | %-20s | %-4s | %-8d\n",
               i + 1,
               p->data.masv,
               hoten,
               p->data.phai,
               dem_diem(p->data.ds_diemthi));
    }

    delete[] arr;
}

// ================== Helpers: bang chon / in MON HOC ==================

static int count_monhoc_nodes(PTRMH r) {
    return dem_monhoc(r);
}

static void fill_monhoc_array(PTRMH r, PTRMH* arr, int& idx) {
    if (!r) return;
    fill_monhoc_array(r->left, arr, idx);
    arr[idx++] = r;
    fill_monhoc_array(r->right, arr, idx);
}

static void print_ds_monhoc_overview(PTRMH root) {
    if (!root) {
        printf("\nChua co mon hoc nao. Hay vao 'Quan ly Mon hoc' de them mon.\n");
        return;
    }
    int n = count_monhoc_nodes(root);
    if (n <= 0) {
        printf("\nChua co mon hoc nao. Hay vao 'Quan ly Mon hoc' de them mon.\n");
        return;
    }

    PTRMH* arr = new PTRMH[n];
    int idx = 0;
    fill_monhoc_array(root, arr, idx);

    printf("\n=== DANH SACH MON HOC ===\n");
    printf("%-4s | %-10s | %-30s | %-6s\n", "STT", "MAMH", "TENMH", "SoCau");
    printf("-----------------------------------------------------------------\n");
    for (int i = 0; i < n; ++i) {
        PTRMH mh = arr[i];
        printf("%-4d | %-10s | %-30s | %-6d\n",
               i + 1,
               mh->data.mamh,
               mh->data.tenmh,
               dem_cau(mh->data.FirstCHT));
    }
    delete[] arr;
}

static bool chon_monhoc_tu_cay(PTRMH root, char* mamh_out) {
    if (!root) {
        printf("\nChua co mon hoc nao. Hay them mon truoc.\n");
        system("pause");
        return false;
    }
    int n = count_monhoc_nodes(root);
    if (n <= 0) {
        printf("\nChua co mon hoc nao. Hay them mon truoc.\n");
        system("pause");
        return false;
    }

    PTRMH* arr = new PTRMH[n];
    int idx = 0;
    fill_monhoc_array(root, arr, idx);

    printf("\n=== DANH SACH MON HOC ===\n");
    printf("%-4s | %-10s | %-30s | %-6s\n", "STT", "MAMH", "TENMH", "SoCau");
    printf("-----------------------------------------------------------------\n");
    for (int i = 0; i < n; ++i) {
        PTRMH mh = arr[i];
        printf("%-4d | %-10s | %-30s | %-6d\n",
               i + 1,
               mh->data.mamh,
               mh->data.tenmh,
               dem_cau(mh->data.FirstCHT));
    }

    int stt;
    if (!read_int("Nhap STT mon hoc (0 = quay lai): ", stt)) {
        delete[] arr;
        return false;
    }

    if (stt == 0) {
        delete[] arr;
        return false;
    }
    if (stt < 1 || stt > n) {
        printf("STT khong hop le.\n");
        system("pause");
        delete[] arr;
        return false;
    }

    su_strncpy(mamh_out, arr[stt - 1]->data.mamh, 16);
    delete[] arr;
    return true;
}

// ================== Helpers: chon MON MA SV DA THI ==================

static bool chon_mon_da_thi_cua_sv(PTRExamLog logs, const char* masv_ci, char* out_mamh, int out_len) {
    const int MAX_MH = 100;
    char mamh_arr[MAX_MH][16];
    int n = 0;

    for (ExamRecord* p = logs; p; p = p->next) {
        if (su_stricmp(p->masv, masv_ci) != 0) continue;

        bool existed = false;
        for (int i = 0; i < n; ++i) {
            if (su_stricmp(mamh_arr[i], p->mamh) == 0) {
                existed = true;
                break;
            }
        }
        if (!existed && n < MAX_MH) {
            su_strncpy(mamh_arr[n], p->mamh, 16);
            ++n;
        }
    }

    if (n == 0) {
        printf("Ban chua co bai thi nao de xem chi tiet.\n");
        return false;
    }

    printf("\n=== CAC MON BAN DA THI ===\n");
    printf("%-4s | %-10s | %-30s | %s\n", "STT", "MAMH", "TENMH", "DIEM");
    printf("---------------------------------------------------------------\n");
    for (int i = 0; i < n; ++i) {
        PTRMH mh = find_monhoc(g_rootMH, mamh_arr[i]);
        const char* tenmh = mh ? mh->data.tenmh : "(khong tim thay ten)";
        ExamRecord* r = find_exam(logs, masv_ci, mamh_arr[i]);
        float diem = r ? r->diem : 0.0f;
        printf("%-4d | %-10s | %-30s | %.1f\n",
               i + 1, mamh_arr[i], tenmh, diem);
    }

    int stt;
    if (!read_int("Chon STT mon (0 = quay lai): ", stt)) {
        return false;
    }

    if (stt == 0) return false;
    if (stt < 1 || stt > n) {
        printf("STT khong hop le.\n");
        system("pause");
        return false;
    }

    su_strncpy(out_mamh, mamh_arr[stt - 1], out_len);
    return true;
}

// ================== MENU QUAN LY LOP ==================

static void menu_quanly_lop() {
    while (!g_exit_app) {
        clearScreen();
        printf("=== QUAN LY LOP ===\n");
        print_ds_lop_overview();
        printf("\n");
        printf("1. Them lop\n");
        printf("2. Xoa lop\n");
        printf("3. In lai danh sach lop\n");
        printf("4. Hoan tac thay doi gan nhat (Undo)\n");
        printf("5. Quay lai\n");
        printf("(ESC: Thoat chuong trinh)\n");
        printf("Chon: ");

        int key = wait_key();
        if (key == KEY_ESC) {
            handle_esc_exit();
            return;
        }

        if (key == '1') {
            char ml[64], tl[128];
            printf("\nNhap MA LOP (toi da 15 ky tu, chi A-Z, 0-9, khong cach. VD: N24DCCN01): ");
            std::fgets(ml, sizeof(ml), stdin); chomp_line(ml);
            normalize_code(ml);

            if (su_strlen(ml) == 0) {
                printf("Loi: MA LOP khong duoc rong.\n");
                system("pause");
                continue;
            }

            printf("Nhap TEN LOP: ");
            std::fgets(tl, sizeof(tl), stdin); chomp_line(tl);
            if (is_blank_str(tl)) {
                printf("Loi: TEN LOP khong duoc rong hoac chi co khoang trang.\n");
                system("pause");
                continue;
            }

            save_undo_snapshot();

            int r = them_lop(g_dslop, ml, tl);
            if (r == 0) {
                char detail[128];
                std::snprintf(detail, sizeof(detail), "Ma lop: %s | Ten lop: %s", ml, tl);
                set_last_op("Them lop", detail);
                printf("Da them lop thanh cong.\n");
                save_all();
            } else if (r == 1) {
                printf("Loi: MA LOP da ton tai. Vui long chon ma khac.\n");
            } else if (r == 2) {
                printf("Loi: Danh sach lop da day (toi da %d lop).\n", MAX_LOP);
            } else if (r == 3) {
                printf("Loi: MA LOP khong hop le (chi chap nhan A-Z, 0-9, khong rong).\n");
            } else {
                printf("Them lop that bai (code=%d).\n", r);
            }
            system("pause");
        }
        else if (key == '2') {
            int idx = chon_lop_index_tu_bang();
            if (idx < 0) continue;
            Lop* lp = g_dslop.nodes[idx];

            char detail[128];
            std::snprintf(detail, sizeof(detail),
                          "Ma lop: %s | Ten lop: %s", lp->malop, lp->tenlop);

            printf("\nBan dang chon xoa lop: %s - %s\n", lp->malop, lp->tenlop);
            printf("Luu y: se KHONG XOA neu lop con sinh vien da thi.\n");
            if (!confirm_dialog("Ban chac chan xoa lop nay")) continue;

            save_undo_snapshot();

            if (xoa_lop(g_dslop, lp->malop)) {
                set_last_op("Xoa lop", detail);
                printf("Da xoa lop.\n");
                save_all();
            } else {
                printf("Khong the xoa lop: hoac lop khong ton tai, hoac co SV da thi.\n");
            }
            system("pause");
        }
        else if (key == '3') {
            clearScreen();
            printf("=== DANH SACH LOP (CAP NHAT) ===\n");
            print_ds_lop_overview();
            system("pause");
        }
        else if (key == '4') {
            menu_undo_action();
        }
        else if (key == '5') {
            return;
        }
    }
}

// ================== MENU QUAN LY SINH VIEN ==================

static void menu_quanly_sv() {
    while (!g_exit_app) {
        clearScreen();
        printf("=== QUAN LY SINH VIEN ===\n");
        printf("Tong so lop hien co: %d\n", g_dslop.n);
        print_ds_lop_overview();
        printf("\n");
        printf("1. Nhap SV vao lop\n");
        printf("2. Sua SV\n");
        printf("3. Xoa SV\n");
        printf("4. In DS SV cua 1 lop\n");
        printf("5. Hoan tac thay doi gan nhat (Undo)\n");
        printf("6. Quay lai\n");
        printf("(ESC: Thoat chuong trinh)\n");
        printf("Chon: ");

        int key = wait_key();
        if (key == KEY_ESC) {
            handle_esc_exit();
            return;
        }

        if (key == '1') {
            int idxLop = chon_lop_index_tu_bang();
            if (idxLop < 0) continue;
            Lop* lop = g_dslop.nodes[idxLop];

            char ms[64], ho[128], ten[64], phai[16], pw[64];

            printf("\nThem SV vao lop %s - %s\n", lop->malop, lop->tenlop);
            printf("MASV (A-Z, 0-9, toi da 15, khong cach. VD: N24DCCN001): ");
            std::fgets(ms, sizeof(ms), stdin); chomp_line(ms);
            normalize_code(ms);

            if (su_strlen(ms) == 0) {
                printf("Loi: MASV khong duoc rong.\n");
                system("pause");
                continue;
            }

            if (masv_exists_in_system(ms)) {
                printf("Loi: MASV da ton tai trong toan he thong.\n");
                system("pause");
                continue;
            }

            printf("Ho: ");
            std::fgets(ho, sizeof(ho), stdin); chomp_line(ho);
            if (is_blank_str(ho)) {
                printf("Loi: Ho khong duoc rong hoac chi co khoang trang.\n");
                system("pause");
                continue;
            }

            printf("Ten: ");
            std::fgets(ten, sizeof(ten), stdin); chomp_line(ten);
            if (is_blank_str(ten)) {
                printf("Loi: Ten khong duoc rong hoac chi co khoang trang.\n");
                system("pause");
                continue;
            }

            printf("Phai (NAM/NU): ");
            std::fgets(phai, sizeof(phai), stdin); chomp_line(phai);

            if (!(su_stricmp(phai, "NAM") == 0 || su_stricmp(phai, "NU") == 0)) {
                printf("Loi: PHAI chi chap nhan 'NAM' hoac 'NU'.\n");
                system("pause");
                continue;
            }

            printf("Password (4-20 ky tu): ");
            std::fgets(pw, sizeof(pw), stdin); chomp_line(pw);
            int lenpw = (int)su_strlen(pw);
            if (lenpw < 4 || lenpw > 20 || is_blank_str(pw)) {
                printf("Loi: PASSWORD phai co do dai tu 4 den 20 ky tu va khong duoc chi co khoang trang.\n");
                system("pause");
                continue;
            }

            save_undo_snapshot();

            int r = them_sv_vao_ds(g_dslop, lop->malop, ms, ho, ten, phai, pw);
            if (r == 0) {
                char detail[128];
                std::snprintf(detail, sizeof(detail),
                              "Lop: %s | MASV: %s | Ho ten: %s %s",
                              lop->malop, ms, ho, ten);
                set_last_op("Them sinh vien", detail);
                printf("Da them sinh vien thanh cong.\n");
                save_all();
            } else if (r == 1) {
                printf("Loi: Khong tim thay lop (co the lop da bi xoa).\n");
            } else if (r == 2) {
                printf("Loi: MASV da ton tai trong toan bo he thong.\n");
            } else if (r == 3) {
                printf("Loi: Du lieu sinh vien khong hop le.\n");
                printf("- MASV chi gom A-Z, 0-9, khong rong.\n");
                printf("- PHAI phai la 'NAM' hoac 'NU'.\n");
                printf("- PASSWORD do dai tu 4 den 20 ky tu.\n");
            } else {
                printf("Them sinh vien that bai (code=%d).\n", r);
            }
            system("pause");
        }
        else if (key == '2') {
            int idxLop = chon_lop_index_tu_bang();
            if (idxLop < 0) continue;
            Lop* lop = g_dslop.nodes[idxLop];

            PTRSV sv = chon_sv_trong_lop(lop);
            if (!sv) continue;

            char ho_cu[128], ten_cu[64], phai_cu[16];
            su_strcpy(ho_cu,   sv->data.ho);
            su_strcpy(ten_cu,  sv->data.ten);
            su_strcpy(phai_cu, sv->data.phai);

            char ho[128], ten[64], phai[16], pw[64];

            printf("\nSua SV: %s - %s %s (%s)\n",
                   sv->data.masv, sv->data.ho, sv->data.ten, sv->data.phai);

            printf("Ho moi (bo trong = giu nguyen: %s): ", sv->data.ho);
            std::fgets(ho, sizeof(ho), stdin); chomp_line(ho);
            if (is_blank_str(ho)) su_strcpy(ho, sv->data.ho);

            printf("Ten moi (bo trong = giu nguyen: %s): ", sv->data.ten);
            std::fgets(ten, sizeof(ten), stdin); chomp_line(ten);
            if (is_blank_str(ten)) su_strcpy(ten, sv->data.ten);

            printf("Phai moi (NAM/NU, bo trong = %s): ", sv->data.phai);
            std::fgets(phai, sizeof(phai), stdin); chomp_line(phai);
            if (is_blank_str(phai)) su_strcpy(phai, sv->data.phai);

            printf("Password moi (4-20 ky tu, bo trong = giu nguyen): ");
            std::fgets(pw, sizeof(pw), stdin); chomp_line(pw);
            if (is_blank_str(pw)) su_strcpy(pw, sv->data.password);

            save_undo_snapshot();

            int r = sua_sv_trong_lop(lop, sv->data.masv, ho, ten, phai, pw);
            if (r == 0) {
                char detail[128];
                std::snprintf(detail, sizeof(detail),
                              "MASV: %s | HoTen: '%s %s' -> '%s %s' | Phai: %s -> %s",
                              sv->data.masv, ho_cu, ten_cu, ho, ten, phai_cu, phai);
                set_last_op("Sua sinh vien", detail);
                printf("Da cap nhat thong tin sinh vien.\n");
                save_all();
            } else if (r == 1) {
                printf("Loi: Khong tim thay sinh vien trong lop.\n");
            } else if (r == 2) {
                printf("Loi: Du lieu moi khong hop le.\n");
                printf("- PHAI phai la 'NAM' hoac 'NU'.\n");
                printf("- PASSWORD do dai tu 4 den 20 ky tu.\n");
            } else {
                printf("Sua sinh vien that bai (code=%d).\n", r);
            }
            system("pause");
        }
        else if (key == '3') {
            int idxLop = chon_lop_index_tu_bang();
            if (idxLop < 0) continue;
            Lop* lop = g_dslop.nodes[idxLop];

            PTRSV sv = chon_sv_trong_lop(lop);
            if (!sv) continue;

            char detail[128];
            std::snprintf(detail, sizeof(detail),
                          "Lop: %s | MASV: %s | Ho ten: %s %s",
                          lop->malop, sv->data.masv, sv->data.ho, sv->data.ten);

            printf("\nBan dang chon xoa SV: %s - %s %s\n",
                   sv->data.masv, sv->data.ho, sv->data.ten);
            printf("Luu y: se KHONG xoa duoc neu SV da co diem/bai thi.\n");
            if (!confirm_dialog("Xoa sinh vien nay?")) continue;

            save_undo_snapshot();

            if (xoa_sv_khoi_lop(lop, sv->data.masv)) {
                set_last_op("Xoa sinh vien", detail);
                printf("Da xoa sinh vien.\n");
                save_all();
            } else {
                printf("Khong xoa duoc: SV co diem/bai thi hoac khong ton tai.\n");
            }
            system("pause");
        }
        else if (key == '4') {
            int idxLop = chon_lop_index_tu_bang();
            if (idxLop < 0) continue;
            Lop* lop = g_dslop.nodes[idxLop];

            clearScreen();
            in_ds_sv_cua_lop(lop);
            system("pause");
        }
        else if (key == '5') {
            menu_undo_action();
        }
        else if (key == '6') {
            return;
        }
    }
}

// ================== MENU QUAN LY MON HOC ==================

static void menu_quanly_monhoc() {
    while (!g_exit_app) {
        clearScreen();
        printf("=== QUAN LY MON HOC ===\n");
        print_ds_monhoc_overview(g_rootMH);
        printf("\n");
        printf("1. Them mon hoc\n");
        printf("2. Sua ten mon hoc\n");
        printf("3. Xoa mon hoc (chan neu con cau hoi)\n");
        printf("4. Hoan tac thay doi gan nhat (Undo)\n");
        printf("5. Quay lai\n");
        printf("(ESC: Thoat chuong trinh)\n");
        printf("Chon: ");

        int key = wait_key();
        if (key == KEY_ESC) {
            handle_esc_exit();
            return;
        }

        if (key == '1') {
            char mm[64], tn[128];
            printf("\nMa MH (A-Z, 0-9, toi da 15, khong cach. VD: CTDLGT): ");
            std::fgets(mm, sizeof(mm), stdin); chomp_line(mm);
            normalize_code(mm);

            if (su_strlen(mm) == 0) {
                printf("Loi: Ma mon hoc khong duoc rong.\n");
                system("pause");
                continue;
            }

            printf("Ten MH: ");
            std::fgets(tn, sizeof(tn), stdin); chomp_line(tn);
            if (is_blank_str(tn)) {
                printf("Loi: Ten mon hoc khong duoc rong hoac chi co khoang trang.\n");
                system("pause");
                continue;
            }

            save_undo_snapshot();

            int r = insert_monhoc(g_rootMH, mm, tn);
            if (r == 0) {
                char detail[128];
                std::snprintf(detail, sizeof(detail),
                              "Ma MH: %s | Ten MH: %s", mm, tn);
                set_last_op("Them mon hoc", detail);
                printf("Da them mon hoc.\n");
                save_all();
            } else if (r == 1) {
                printf("Loi: Ma mon hoc da ton tai.\n");
            } else if (r == 2) {
                printf("Loi: Ma mon hoc khong hop le (chi A-Z, 0-9).\n");
            } else {
                printf("Them mon hoc that bai (code=%d).\n", r);
            }
            system("pause");
        }
        else if (key == '2') {
            char mamh[16];
            if (!chon_monhoc_tu_cay(g_rootMH, mamh)) continue;
            PTRMH mh = find_monhoc(g_rootMH, mamh);
            if (!mh) { printf("Khong tim thay mon hoc.\n"); system("pause"); continue; }

            char ten_cu[128];
            std::snprintf(ten_cu, sizeof(ten_cu), "%s", mh->data.tenmh);

            char tn[128];
            printf("\nTen MH moi: ");
            std::fgets(tn, sizeof(tn), stdin); chomp_line(tn);
            if (is_blank_str(tn)) {
                printf("Loi: Ten mon hoc khong duoc rong hoac chi co khoang trang.\n");
                system("pause");
                continue;
            }

            save_undo_snapshot();

            int r = update_monhoc_name(g_rootMH, mamh, tn);
            if (r == 0) {
                char detail[128];
                std::snprintf(detail, sizeof(detail),
                              "MAMH: %s | Ten: '%s' -> '%s'", mamh, ten_cu, tn);
                set_last_op("Sua ten mon hoc", detail);
                printf("Da cap nhat ten mon hoc.\n");
                save_all();
            } else {
                printf("Khong tim thay mon hoc.\n");
            }
            system("pause");
        }
        else if (key == '3') {
            char mamh[16];
            if (!chon_monhoc_tu_cay(g_rootMH, mamh)) continue;
            PTRMH mh = find_monhoc(g_rootMH, mamh);
            if (!mh) { printf("Khong tim thay mon hoc.\n"); system("pause"); continue; }

            char detail[128];
            std::snprintf(detail, sizeof(detail),
                          "MAMH: %s | Ten MH: %s", mh->data.mamh, mh->data.tenmh);

            printf("\nBan dang chon xoa mon hoc: %s - %s\n", mh->data.mamh, mh->data.tenmh);
            printf("Luu y: SE KHONG XOA neu mon con cau hoi.\n");
            if (!confirm_dialog("Xac nhan xoa mon hoc nay")) continue;

            save_undo_snapshot();

            if (delete_monhoc_safe(g_rootMH, mamh)) {
                set_last_op("Xoa mon hoc", detail);
                printf("Da xoa mon hoc.\n");
                save_all();
            } else {
                printf("Khong the xoa (mon khong ton tai hoac con cau hoi).\n");
            }
            system("pause");
        }
        else if (key == '4') {
            menu_undo_action();
        }
        else if (key == '5') {
            return;
        }
    }
}

// ================== MENU QUAN LY CAU HOI ==================

static void menu_quanly_cauhoi() {
    while (!g_exit_app) {
        clearScreen();
        printf("=== QUAN LY CAU HOI THEO MON ===\n");
        printf("Tong quan mon hoc (so cau hien co):\n");
        print_ds_monhoc_overview(g_rootMH);
        printf("\n");
        printf("1. Them cau hoi\n");
        printf("2. Xoa cau hoi (chan neu da thi)\n");
        printf("3. In ds cau cua 1 mon\n");
        printf("4. Hoan tac thay doi gan nhat (Undo)\n");
        printf("5. Quay lai\n");
        printf("(ESC: Thoat chuong trinh)\n");
        printf("Chon: ");

        int key = wait_key();
        if (key == KEY_ESC) {
            handle_esc_exit();
            return;
        }

        if (key == '1') {
            char mamh[16];
            if (!chon_monhoc_tu_cay(g_rootMH, mamh)) continue;
            PTRMH mh = find_monhoc(g_rootMH, mamh);
            if (!mh) { printf("Khong tim thay mon hoc.\n"); system("pause"); continue; }

            save_undo_snapshot();

            CauHoi ch;
            // NEW: O(1) cap phat ID (counter++) nhung "nhin ngau nhien" va KHONG TRUNG
            ch.id = new_question_id_auto();

            printf("\nNhap noi dung cau hoi:\n");
            std::fgets(ch.noidung, sizeof(ch.noidung), stdin); chomp_line(ch.noidung);
            if (is_blank_str(ch.noidung)) {
                printf("Loi: Noi dung cau hoi khong duoc rong hoac chi co khoang trang.\n");
                system("pause");
                continue;
            }

            // NEW: bat buoc 4 dap an KHAC NHAU (2/3/4 trung deu chan)
            while (true) {
                printf("Phuong an A: ");
                std::fgets(ch.A, sizeof(ch.A), stdin); chomp_line(ch.A);

                printf("Phuong an B: ");
                std::fgets(ch.B, sizeof(ch.B), stdin); chomp_line(ch.B);

                printf("Phuong an C: ");
                std::fgets(ch.C, sizeof(ch.C), stdin); chomp_line(ch.C);

                printf("Phuong an D: ");
                std::fgets(ch.D, sizeof(ch.D), stdin); chomp_line(ch.D);

                if (!answers_all_distinct(ch.A, ch.B, ch.C, ch.D)) {
                    printf("\nLoi: 4 dap an phai KHAC nhau va khong duoc rong.\n");
                    printf("Chi can 2 dap an trung (VD: A=4, B=4) la BI CHAN.\n");
                    printf("Vui long nhap lai 4 dap an!\n\n");
                    continue;
                }
                break;
            }

            char dd[8];
            do {
                printf("Dap an dung (A/B/C/D): ");
                std::fgets(dd, sizeof(dd), stdin); chomp_line(dd);
                ch.dapan = dd[0];
                if (ch.dapan >= 'a' && ch.dapan <= 'z') ch.dapan = (char)(ch.dapan - 'a' + 'A');
            } while (!(ch.dapan == 'A' || ch.dapan == 'B' || ch.dapan == 'C' || ch.dapan == 'D'));

            add_cau_hoi(mh->data.FirstCHT, ch);

            char detail[128];
            char noidung_short[40];
            std::snprintf(noidung_short, sizeof(noidung_short), "%.35s%s",
                          ch.noidung, (su_strlen(ch.noidung) > 35) ? "..." : "");
            std::snprintf(detail, sizeof(detail),
                          "Mon: %s | ID: %d | ND: %s",
                          mh->data.mamh, ch.id, noidung_short);
            set_last_op("Them cau hoi", detail);

            printf("Da them cau hoi (id=%d) vao mon %s.\n", ch.id, mh->data.mamh);
            save_all();
            system("pause");
        }
        else if (key == '2') {
            char mamh[16];
            if (!chon_monhoc_tu_cay(g_rootMH, mamh)) continue;
            PTRMH mh = find_monhoc(g_rootMH, mamh);
            if (!mh) { printf("Khong tim thay mon hoc.\n"); system("pause"); continue; }

            char idbuf[64];
            printf("\nID cau muon xoa: ");
            std::fgets(idbuf, sizeof(idbuf), stdin); chomp_line(idbuf);
            int id = std::atoi(idbuf);

            PTRCH q = mh->data.FirstCHT;
            char detail[128];
            if (q) {
                while (q && q->data.id != id) q = q->next;
            }

            if (q) {
                char noidung_short[40];
                std::snprintf(noidung_short, sizeof(noidung_short), "%.35s%s",
                              q->data.noidung, (su_strlen(q->data.noidung) > 35) ? "..." : "");
                std::snprintf(detail, sizeof(detail),
                              "Mon: %s | ID: %d | ND: %s",
                              mh->data.mamh, id, noidung_short);
            } else {
                std::snprintf(detail, sizeof(detail),
                              "Mon: %s | ID: %d", mh->data.mamh, id);
            }

            if (examlog_contains_question(g_logs, mamh, id)) {
                printf("Khong the xoa: cau hoi da duoc dung trong bai thi.\n");
            } else {
                if (!confirm_dialog("Xac nhan xoa cau hoi nay")) {
                    system("pause");
                    continue;
                }

                save_undo_snapshot();

                if (remove_cau_by_id(mh->data.FirstCHT, id)) {
                    set_last_op("Xoa cau hoi", detail);
                    printf("Da xoa cau hoi.\n");
                    save_all();
                } else {
                    printf("Khong tim thay cau hoi co ID do.\n");
                }
            }
            system("pause");
        }
        else if (key == '3') {
            char mamh[16];
            if (!chon_monhoc_tu_cay(g_rootMH, mamh)) continue;
            PTRMH mh = find_monhoc(g_rootMH, mamh);
            if (!mh) { printf("Khong tim thay mon hoc.\n"); system("pause"); continue; }

            printf("\n=== Cau hoi cua mon %s - %s ===\n", mh->data.mamh, mh->data.tenmh);
            for (PTRCH p = mh->data.FirstCHT; p; p = p->next) {
                printf("ID=%d | %s (DA: %c)\n", p->data.id, p->data.noidung, p->data.dapan);
            }
            system("pause");
        }
        else if (key == '4') {
            menu_undo_action();
        }
        else if (key == '5') {
            return;
        }
    }
}

// ================== MENU SINH VIEN ==================

static void menu_thi_sv() {
    char ms[64], pw[64];
    printf("=== DANG NHAP SINH VIEN ===\n");
    printf("(MASV: toi da 15 ky tu, chi chu va so, khong khoang trang. VD: N24DCCN001)\n");
    printf("MASV: ");
    std::fgets(ms, sizeof(ms), stdin); chomp_line(ms);
    printf("Password: ");
    std::fgets(pw, sizeof(pw), stdin); chomp_line(pw);

    char tms[16]; su_strncpy(tms, ms, 16); normalize_code(tms);

    PTRSV sv = NULL;
    Lop* lop = NULL;
    bool foundMasv = false;

    for (int i = 0; i < g_dslop.n; ++i) {
        for (PTRSV p = g_dslop.nodes[i]->FirstSV; p; p = p->next) {
            if (su_stricmp(p->data.masv, tms) == 0) {
                foundMasv = true;
                if (su_strcmp(p->data.password, pw) == 0) {
                    sv  = p;
                    lop = g_dslop.nodes[i];
                    break;
                }
            }
        }
        if (sv) break;
    }

    if (!foundMasv) {
        printf("Ma sinh vien khong ton tai trong he thong.\n");
        printf("Vui long kiem tra lai MASV hoac lien he giang vien.\n");
        system("pause");
        return;
    }
    if (!sv) {
        printf("Mat khau khong dung. Vui long thu lai.\n");
        system("pause");
        return;
    }

    while (!g_exit_app) {
        clearScreen();
        printf("=== SINH VIEN: %s - %s %s (%s) ===\n",
               sv->data.masv, sv->data.ho, sv->data.ten, lop->tenlop);
        printf("1. Thi trac nghiem\n");
        printf("2. Xem diem cac mon\n");
        printf("3. Xem chi tiet bai thi 1 mon\n");
        printf("4. Quay lai\n");
        printf("(ESC: Thoat chuong trinh)\n");
        printf("Chon: ");

        int key = wait_key();
        if (key == KEY_ESC) {
            handle_esc_exit();
            return;
        }

        if (key == '1') {
            char mamh[16];
            if (!chon_monhoc_tu_cay(g_rootMH, mamh)) continue;

            PTRMH mh = find_monhoc(g_rootMH, mamh);
            if (!mh) {
                printf("Loi noi bo: khong tim thay mon hoc.\n");
                system("pause");
                continue;
            }

            int tongCau = dem_cau(mh->data.FirstCHT);
            if (tongCau <= 0) {
                printf("Mon hoc nay chua co cau hoi nao, khong the thi.\n");
                system("pause");
                continue;
            }

            printf("\nMon %s - %s dang co %d cau hoi.\n",
                   mh->data.mamh, mh->data.tenmh, tongCau);
            printf("Ban chi duoc chon so cau thi tu 1 den %d.\n", tongCau);

            char sbuf[64], pbuf[64];
            printf("\nSo cau muon thi: ");
            std::fgets(sbuf, sizeof(sbuf), stdin); chomp_line(sbuf);
            printf("So phut lam bai: ");
            std::fgets(pbuf, sizeof(pbuf), stdin); chomp_line(pbuf);
            int soCau  = std::atoi(sbuf);
            int soPhut = std::atoi(pbuf);

            if (soCau <= 0) {
                printf("So cau thi khong hop le (phai > 0).\n");
                system("pause");
                continue;
            }
            if (soCau > tongCau) {
                printf("Mon nay chi co %d cau hoi. Vui long nhap so cau <= %d.\n", tongCau, tongCau);
                system("pause");
                continue;
            }
            if (soPhut <= 0) {
                printf("So phut lam bai phai > 0.\n");
                system("pause");
                continue;
            }

            int r = thuc_hien_thi(g_dslop, g_rootMH, g_logs, sv->data.masv, mamh, soCau, soPhut);
            if (r == 1)      printf("Loi: Khong tim thay sinh vien.\n");
            else if (r == 2) printf("Loi: Khong tim thay mon hoc.\n");
            else if (r == 3) printf("Loi: So cau thi khong hop le (phai > 0).\n");
            else if (r == 4) printf("Loi: Mon hoc khong du so cau yeu cau.\n");
            else if (r == 0) {
                printf("Da ket thuc bai thi.\n");
                save_all();
            }
            system("pause");
        }
        else if (key == '2') {
            printf("\n=== DIEM CUA BAN ===\n");
            if (!sv->data.ds_diemthi) {
                printf("Ban chua thi mon nao.\n");
            } else {
                for (PTRDiemThi d = sv->data.ds_diemthi; d; d = d->next) {
                    PTRMH mh = find_monhoc(g_rootMH, d->data.mamh);
                    const char* ten = (mh ? mh->data.tenmh : "(khong tim thay mon)");
                    printf("%s - %-25s : %.1f\n", d->data.mamh, ten, d->data.diem);
                }
            }
            system("pause");
        }
        else if (key == '3') {
            char mamh[16];
            if (!chon_mon_da_thi_cua_sv(g_logs, sv->data.masv, mamh, 16)) {
                system("pause");
                continue;
            }
            in_chi_tiet_bai_thi(g_logs, sv->data.masv, mamh);
            system("pause");
        }
        else if (key == '4') {
            return;
        }
    }
}

// ================== MENU GIANG VIEN ==================

static void menu_gv() {
    while (!g_exit_app) {
        clearScreen();
        printf("=== QUAN TRI (GIANG VIEN) ===\n");
        printf("1. Quan ly Lop\n");
        printf("2. Quan ly Sinh Vien\n");
        printf("3. Quan ly Mon hoc\n");
        printf("4. Quan ly Cau hoi\n");
        printf("5. Quay lai\n");
        printf("(ESC: Thoat chuong trinh)\n");
        printf("Chon: ");

        int key = wait_key();
        if (key == KEY_ESC) {
            handle_esc_exit();
            return;
        }

        if (key == '1')      menu_quanly_lop();
        else if (key == '2') menu_quanly_sv();
        else if (key == '3') menu_quanly_monhoc();
        else if (key == '4') menu_quanly_cauhoi();
        else if (key == '5') return;
    }
}

// ================== HAM CHAY CHINH ==================

void run_console_app() {
    rng_seed_once();
    load_all();
    g_exit_app      = false;
    g_has_undo_snap = false;
    g_last_op_title[0]   = '\0';
    g_last_op_detail[0]  = '\0';

    while (!g_exit_app) {
        clearScreen();
        printf("=== HE THONG THI TRAC NGHIEM ===\n");
        printf("1. Dang nhap GV (tai khoan: GV / GV)\n");
        printf("2. Dang nhap SV\n");
        printf("3. Thoat\n");
        printf("(ESC: Thoat chuong trinh)\n");
        printf("Chon: ");

        int key = wait_key();

        if (key == KEY_ESC || key == '3') {
            handle_esc_exit();
            break;
        }

        if (key == '1') {
            char u[64], p[64];
            printf("\n=== DANG NHAP GIANG VIEN ===\n");
            printf("User (GV): "); std::fgets(u, sizeof(u), stdin); chomp_line(u);
            printf("Pass: ");       std::fgets(p, sizeof(p), stdin); chomp_line(p);
            if (su_stricmp(u, "GV") == 0 && su_stricmp(p, "GV") == 0) {
                menu_gv();
            } else {
                printf("Sai tai khoan hoac mat khau GV (mac dinh: GV / GV).\n");
                system("pause");
            }
        }
        else if (key == '2') {
            menu_thi_sv();
        }
    }

    free_ds_lop(g_dslop);
    free_all_monhoc(g_rootMH);
    free_examlog(g_logs);
}
