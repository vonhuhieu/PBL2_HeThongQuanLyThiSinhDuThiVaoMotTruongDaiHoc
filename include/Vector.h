#ifndef VECTOR_H
#define VECTOR_H

#include <iostream>
#include <stdexcept>

/**
 * @brief Cấu trúc dữ liệu Vector (Template mảng động tự cài đặt)
 * Đáp ứng yêu cầu chuẩn mực của Đồ án cơ sở lập trình PBL2
 */
template <typename T>
class Vector {
private:
    T* data;            // Vùng nhớ động lưu các phần tử
    int m_size;         // Số lượng phần tử thực tế
    int m_capacity;     // Dung lượng bộ nhớ được cấp phát

    void reallocate(int newCapacity) {
        if (newCapacity < m_size) {
            newCapacity = m_size;
        }
        if (newCapacity == 0) newCapacity = 1;

        T* newData = new T[newCapacity];
        for (int i = 0; i < m_size; ++i) {
            newData[i] = data[i];
        }

        delete[] data;
        data = newData;
        m_capacity = newCapacity;
    }

    // Thuật toán QuickSort nội bộ sử dụng con trỏ hàm so sánh
    void quickSort(int left, int right, bool (*compare)(const T&, const T&)) {
        if (left >= right) return;

        T pivot = data[(left + right) / 2];
        int i = left;
        int j = right;

        while (i <= j) {
            while (compare(data[i], pivot)) i++;
            while (compare(pivot, data[j])) j--;

            if (i <= j) {
                T temp = data[i];
                data[i] = data[j];
                data[j] = temp;
                i++;
                j--;
            }
        }

        if (left < j) quickSort(left, j, compare);
        if (i < right) quickSort(i, right, compare);
    }

public:
    // 1. Constructor mặc định
    Vector() : data(nullptr), m_size(0), m_capacity(0) {
        reallocate(10);
    }

    // 2. Constructor với dung lượng ban đầu
    explicit Vector(int initialCapacity) : data(nullptr), m_size(0), m_capacity(0) {
        if (initialCapacity < 1) initialCapacity = 10;
        reallocate(initialCapacity);
    }

    // 3. Copy Constructor (Deep copy)
    Vector(const Vector<T>& other) : data(nullptr), m_size(other.m_size), m_capacity(other.m_capacity) {
        data = new T[m_capacity];
        for (int i = 0; i < m_size; ++i) {
            data[i] = other.data[i];
        }
    }

    // 4. Copy Assignment Operator
    Vector<T>& operator=(const Vector<T>& other) {
        if (this != &other) {
            delete[] data;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            data = new T[m_capacity];
            for (int i = 0; i < m_size; ++i) {
                data[i] = other.data[i];
            }
        }
        return *this;
    }

    // 5. Destructor
    ~Vector() {
        delete[] data;
        data = nullptr;
        m_size = 0;
        m_capacity = 0;
    }

    // Lấy số lượng phần tử
    int size() const {
        return m_size;
    }

    // Lấy dung lượng tối đa hiện tại
    int capacity() const {
        return m_capacity;
    }

    // Kiểm tra rỗng
    bool empty() const {
        return m_size == 0;
    }

    // Thêm phần tử vào cuối danh sách
    void push_back(const T& element) {
        if (m_size >= m_capacity) {
            reallocate(m_capacity == 0 ? 10 : m_capacity * 2);
        }
        data[m_size++] = element;
    }

    // Xóa phần tử cuối cùng
    void pop_back() {
        if (m_size > 0) {
            m_size--;
        }
    }

    // Chèn phần tử tại vị trí index
    void insert(int index, const T& element) {
        if (index < 0 || index > m_size) {
            throw std::out_of_range("Vi tri chen nam ngoai pham vi hop le!");
        }
        if (m_size >= m_capacity) {
            reallocate(m_capacity == 0 ? 10 : m_capacity * 2);
        }
        for (int i = m_size; i > index; --i) {
            data[i] = data[i - 1];
        }
        data[index] = element;
        m_size++;
    }

    // Xóa phần tử tại vị trí index
    void erase(int index) {
        if (index < 0 || index >= m_size) {
            throw std::out_of_range("Vi tri xoa nam ngoai pham vi hop le!");
        }
        for (int i = index; i < m_size - 1; ++i) {
            data[i] = data[i + 1];
        }
        m_size--;
    }

    // Truy cập phần tử qua toán tử []
    T& operator[](int index) {
        return data[index];
    }

    const T& operator[](int index) const {
        return data[index];
    }

    // Truy cập phần tử an toàn có kiểm tra biên
    T& at(int index) {
        if (index < 0 || index >= m_size) {
            throw std::out_of_range("Chi so vuot qua gioi han cua Vector!");
        }
        return data[index];
    }

    const T& at(int index) const {
        if (index < 0 || index >= m_size) {
            throw std::out_of_range("Chi so vuot qua gioi han cua Vector!");
        }
        return data[index];
    }

    // Xóa toàn bộ phần tử
    void clear() {
        m_size = 0;
    }

    // Dự trữ trước dung lượng bộ nhớ
    void reserve(int newCapacity) {
        if (newCapacity > m_capacity) {
            reallocate(newCapacity);
        }
    }

    // Sắp xếp các phần tử bằng thuật toán QuickSort
    void sort(bool (*compare)(const T&, const T&)) {
        if (m_size > 1) {
            quickSort(0, m_size - 1, compare);
        }
    }
};

#endif // VECTOR_H
