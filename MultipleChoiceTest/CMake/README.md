
# **ĐỀ TÀI MÔN CẤU TRÚC DỮ LIỆU & GIẢI THUẬT**
**KHÓA 2024 - CNTT CLC**

## **3. Thi trắc nghiệm**

### **Tổ chức các danh sách**

1. **Danh sách Môn học**  
   - **Cấu trúc**: **Cây nhị phân tìm kiếm cân bằng (AVL)**  
   - **Thuộc tính**:  
     - **`MAMH` (C15)**  
     - **`TENMH`**  
     - **`ds câu hỏi thi`**

2. **Danh sách Câu hỏi thi**  
   - **Cấu trúc**: **Danh sách liên kết đơn**  
   - **Thuộc tính**:  
     - **`Id`**  
     - **`Nội dung`**  
     - **`A`**, **`B`**, **`C`**, **`D`**  
     - **`Đáp án`**

3. **Danh sách Lớp**  
   - **Cấu trúc**: **Mảng con trỏ (tối đa 10000 lớp)**  
   - **Thuộc tính**:  
     - **`MALOP` (C15)**  
     - **`TENLOP`**  
     - **`con trỏ DSSV`**

4. **Danh sách Sinh viên**  
   - **Cấu trúc**: **Danh sách liên kết đơn**  
   - **Thuộc tính**:  
     - **`MASV` (C15)**  
     - **`HO`**, **`TEN`**, **`PHAI`**  
     - **`password`**  
     - **`con trỏ ds_diemthi`**

5. **Danh sách Điểm thi**  
   - **Cấu trúc**: **Danh sách liên kết đơn**  
   - **Thuộc tính**:  
     - **`MAMH`**  
     - **`Diem` (làm tròn 1 số sau dấu chấm)**

---

## **Các chức năng của chương trình**

### **a) Đăng nhập**  
- **Đăng nhập** dựa vào **mã sinh viên** và **password**.  
- Nếu tài khoản đăng nhập là **GV**, pass là **GV** thì sẽ **có toàn quyền**.

### **b) NhapLop**  
- **Nhập lớp**.

### **c) In ds lớp**  
- **In danh sách lớp**.

### **d) Nhập sinh viên của lớp**  
- **Nhập** vào **mã lớp** trước, sau đó **nhập các sinh viên** vào lớp đó.

### **e) Nhập môn học**  
- Cho phép **cập nhật** (**thêm / xóa / hiệu chỉnh**) **thông tin của môn học**.

### **f) Nhập câu hỏi thi**  
- **`Id`** là **số ngẫu nhiên** do chương trình tự tạo.  
- **Không được xóa** câu hỏi thi **khi đã nằm trong** 1 bài thi trắc nghiệm của sinh viên.

### **g) Thi Trắc nghiệm**  
- Trước khi thi hỏi người thi **môn thi**, **số câu hỏi thi**, **số phút thi**.  
- Sau đó **lấy ngẫu nhiên** các câu hỏi trong **danh sách câu hỏi thi của môn**.

### **h) In chi tiết các câu hỏi đã thi**  
- **In chi tiết** các câu hỏi **đã thi 1 môn học** của **1 sinh viên**.

### **i) In bảng điểm thi trắc nghiệm**  
- **In bảng điểm** thi trắc nghiệm **môn học của 1 lớp** *(nếu có sinh viên chưa thi thì ghi **“Chưa thi”**)*.

---

## **Lưu ý**  
- Chương trình cho phép **lưu các danh sách vào file**.  
- **Kiểm tra các điều kiện** làm dữ liệu bị sai.  
- **Sinh viên** có thể **tự thiết kế thêm danh sách** để đáp ứng yêu cầu của đề tài.
=======
# **ĐỀ TÀI MÔN CẤU TRÚC DỮ LIỆU & GIẢI THUẬT**
**KHÓA 2023 - CNTT CLC**

## **3. Thi trắc nghiệm**

### **Tổ chức các danh sách**

1. **Danh sách Môn học**  
   - **Cấu trúc**: **Cây nhị phân tìm kiếm cân bằng (AVL)**  
   - **Thuộc tính**:  
     - **`MAMH` (C15)**  
     - **`TENMH`**  
     - **`ds câu hỏi thi`**

2. **Danh sách Câu hỏi thi**  
   - **Cấu trúc**: **Danh sách liên kết đơn**  
   - **Thuộc tính**:  
     - **`Id`**  
     - **`Nội dung`**  
     - **`A`**, **`B`**, **`C`**, **`D`**  
     - **`Đáp án`**

3. **Danh sách Lớp**  
   - **Cấu trúc**: **Mảng con trỏ (tối đa 10000 lớp)**  
   - **Thuộc tính**:  
     - **`MALOP` (C15)**  
     - **`TENLOP`**  
     - **`con trỏ DSSV`**

4. **Danh sách Sinh viên**  
   - **Cấu trúc**: **Danh sách liên kết đơn**  
   - **Thuộc tính**:  
     - **`MASV` (C15)**  
     - **`HO`**, **`TEN`**, **`PHAI`**  
     - **`password`**  
     - **`con trỏ ds_diemthi`**

5. **Danh sách Điểm thi**  
   - **Cấu trúc**: **Danh sách liên kết đơn**  
   - **Thuộc tính**:  
     - **`MAMH`**  
     - **`Diem` (làm tròn 1 số sau dấu chấm)**

---

## **Các chức năng của chương trình**

### **a) Đăng nhập**  
- **Đăng nhập** dựa vào **mã sinh viên** và **password**.  
- Nếu tài khoản đăng nhập là **GV**, pass là **GV** thì sẽ **có toàn quyền**.

### **b) NhapLop**  
- **Nhập lớp**.

### **c) In ds lớp**  
- **In danh sách lớp**.

### **d) Nhập sinh viên của lớp**  
- **Nhập** vào **mã lớp** trước, sau đó **nhập các sinh viên** vào lớp đó.

### **e) Nhập môn học**  
- Cho phép **cập nhật** (**thêm / xóa / hiệu chỉnh**) **thông tin của môn học**.

### **f) Nhập câu hỏi thi**  
- **`Id`** là **số ngẫu nhiên** do chương trình tự tạo.  
- **Không được xóa** câu hỏi thi **khi đã nằm trong** 1 bài thi trắc nghiệm của sinh viên.

### **g) Thi Trắc nghiệm**  
- Trước khi thi hỏi người thi **môn thi**, **số câu hỏi thi**, **số phút thi**.  
- Sau đó **lấy ngẫu nhiên** các câu hỏi trong **danh sách câu hỏi thi của môn**.

### **h) In chi tiết các câu hỏi đã thi**  
- **In chi tiết** các câu hỏi **đã thi 1 môn học** của **1 sinh viên**.

### **i) In bảng điểm thi trắc nghiệm**  
- **In bảng điểm** thi trắc nghiệm **môn học của 1 lớp** *(nếu có sinh viên chưa thi thì ghi **“Chưa thi”**)*.

---

## **Lưu ý**  
- Chương trình cho phép **lưu các danh sách vào file**.  
- **Kiểm tra các điều kiện** làm dữ liệu bị sai.  
- **Sinh viên** có thể **tự thiết kế thêm danh sách** để đáp ứng yêu cầu của đề tài.

