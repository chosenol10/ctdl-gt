// monhoc_avl.cpp
#include "monhoc_avl.h"
#include "string_utils.h"
#include "cauhoi.h"
#include <cstdlib>

static int height(PTRMH p) { return p ? p->data.height : 0; }
static int max2(int a, int b) { return a > b ? a : b; }

static void update_height(PTRMH p) {
    if (p) p->data.height = 1 + max2(height(p->left), height(p->right));
}
static int balance_factor(PTRMH p) {
    return p ? height(p->left) - height(p->right) : 0;
}

static PTRMH rotate_right(PTRMH y) {
    PTRMH x = y->left;
    PTRMH T2 = x->right;
    x->right = y;
    y->left = T2;
    update_height(y);
    update_height(x);
    return x;
}
static PTRMH rotate_left(PTRMH x) {
    PTRMH y = x->right;
    PTRMH T2 = y->left;
    y->left = x;
    x->right = T2;
    update_height(x);
    update_height(y);
    return y;
}

void init_avl(PTRMH& root) { root = NULL; }

static PTRMH new_node_mh(const char* mamh_ci, const char* tenmh_norm) {
    PTRMH node = new NodeMH;
    su_strncpy(node->data.mamh, mamh_ci, 16);
    su_strncpy(node->data.tenmh, tenmh_norm, 51);
    node->data.height = 1;
    node->data.FirstCHT = NULL;
    node->left = node->right = NULL;
    return node;
}

static PTRMH insert_rec(PTRMH node, const char* mamh_ci, const char* tenmh_norm, int& status) {
    if (!node) { status = 0; return new_node_mh(mamh_ci, tenmh_norm); }
    int cmp = su_stricmp(mamh_ci, node->data.mamh);
    if (cmp == 0) { status = 1; return node; } // trùng
    else if (cmp < 0) node->left = insert_rec(node->left, mamh_ci, tenmh_norm, status);
    else node->right = insert_rec(node->right, mamh_ci, tenmh_norm, status);

    update_height(node);
    int bf = balance_factor(node);

    // 4 cases
    if (bf > 1 && su_stricmp(mamh_ci, node->left->data.mamh) < 0) return rotate_right(node);
    if (bf < -1 && su_stricmp(mamh_ci, node->right->data.mamh) > 0) return rotate_left(node);
    if (bf > 1 && su_stricmp(mamh_ci, node->left->data.mamh) > 0) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }
    if (bf < -1 && su_stricmp(mamh_ci, node->right->data.mamh) < 0) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }
    return node;
}

int insert_monhoc(PTRMH& root, const char* mamh, const char* tenmh) {
    char mm[16]; su_strncpy(mm, mamh, 16); normalize_code(mm);
    if (!is_code_alnum_upper(mm)) return 2;
    char tn[51]; su_strncpy(tn, tenmh, 51); normalize_name(tn);
    int status = 0;
    root = insert_rec(root, mm, tn, status);
    return status; // 0 OK; 1 trùng
}

PTRMH find_monhoc(PTRMH root, const char* mamh_ci) {
    if (!root) return NULL;
    int c = su_stricmp(mamh_ci, root->data.mamh);
    if (c == 0) return root;
    if (c < 0) return find_monhoc(root->left, mamh_ci);
    return find_monhoc(root->right, mamh_ci);
}

int update_monhoc_name(PTRMH root, const char* mamh_ci, const char* tenmh) {
    PTRMH p = find_monhoc(root, mamh_ci);
    if (!p) return 1;
    char tn[51]; su_strncpy(tn, tenmh, 51); normalize_name(tn);
    su_strncpy(p->data.tenmh, tn, 51);
    return 0;
}

static PTRMH min_value_node(PTRMH node) {
    PTRMH cur = node;
    while (cur && cur->left) cur = cur->left;
    return cur;
}

static PTRMH delete_rec(PTRMH root, const char* mamh_ci, bool& removed, bool& blocked) {
    if (!root) return NULL;
    int cmp = su_stricmp(mamh_ci, root->data.mamh);
    if (cmp < 0) root->left = delete_rec(root->left, mamh_ci, removed, blocked);
    else if (cmp > 0) root->right = delete_rec(root->right, mamh_ci, removed, blocked);
    else {
        // Chặn nếu còn câu hỏi
        if (root->data.FirstCHT != NULL) { blocked = true; return root; }
        // Xoá nút
        removed = true;
        if (!root->left || !root->right) {
            PTRMH temp = root->left ? root->left : root->right;
            delete root;
            return temp;
        } else {
            PTRMH succ = min_value_node(root->right);
            // move succ data
            MonHoc tmp = root->data;
            root->data = succ->data;
            succ->data = tmp;
            // del succ key (root->data.mamh after swap)
            root->right = delete_rec(root->right, mamh_ci, removed, blocked);
        }
    }

    if (!root) return NULL;
    update_height(root);
    int bf = balance_factor(root);

    // balance
    if (bf > 1 && balance_factor(root->left) >= 0) return rotate_right(root);
    if (bf > 1 && balance_factor(root->left) < 0) {
        root->left = rotate_left(root->left);
        return rotate_right(root);
    }
    if (bf < -1 && balance_factor(root->right) <= 0) return rotate_left(root);
    if (bf < -1 && balance_factor(root->right) > 0) {
        root->right = rotate_right(root->right);
        return rotate_left(root);
    }
    return root;
}

bool delete_monhoc_safe(PTRMH& root, const char* mamh_ci) {
    bool removed = false, blocked = false;
    root = delete_rec(root, mamh_ci, removed, blocked);
    if (blocked) return false; // còn câu hỏi
    return removed;
}

void traverse_inorder(PTRMH root, void (*visit)(const MonHoc&)) {
    if (!root) return;
    traverse_inorder(root->left, visit);
    visit(root->data);
    traverse_inorder(root->right, visit);
}

void free_all_monhoc(PTRMH& root) {
    if (!root) return;
    free_all_monhoc(root->left);
    free_all_monhoc(root->right);
    // free danh sách câu hỏi của môn
    PTRCH h = root->data.FirstCHT;
    while (h) {
        PTRCH q = h->next;
        delete h;
        h = q;
    }
    delete root;
    root = NULL;
}
