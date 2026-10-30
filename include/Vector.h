#ifndef VECTOR_H
#define VECTOR_H

#include <iostream>
#include <stdexcept>
#include <utility>

/**
 * @brief Cấu trúc dữ liệu Vector (Template mảng động tự cài đặt)
 * Đáp ứng chuẩn mực đồ án cơ sở lập trình PBL2
 * Tự quản lý bộ nhớ động theo chuẩn Big-5 (Rule of Five)
 * Tích hợp giải thuật sắp xếp nhanh QuickSort nội bộ
 */
template <typename T>
class Vector {
private:
    T* data;            // Vùng nhớ động lưu trữ các phần tử
    size_t m_size;      // Số lượng phần tử thực tế hiện có
    size_t m_capacity;  // Dung lượng bộ nhớ được cấp phát tối đa

    // Tái cấp phát bộ nhớ khi đầy
    void reallocate(size_t newCapacity) {
        if (newCapacity < m_size) {
            newCapacity = m_size;
        }
        if (newCapacity == 0) newCapacity = 1;

        T* newData = new T[newCapacity];
        for (size_t i = 0; i < m_size; ++i) {
            newData[i] = std::move(data[i]);
        }

        delete[] data;
        data = newData;
        m_capacity = newCapacity;
    }

    // Thuật toán QuickSort nội bộ nhận con trỏ hàm so sánh
    void quickSort(int left, int right, bool (*compare)(const T&, const T&)) {
        if (left >= right) return;

        T pivot = data[(left + right) / 2];
        int i = left;
        int j = right;

        while (i <= j) {
            while (compare(data[i], pivot)) i++;
            while (compare(pivot, data[j])) j--;

            if (i <= j) {
                T temp = std::move(data[i]);
                data[i] = std::move(data[j]);
                data[j] = std::move(temp);
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
    explicit Vector(size_t initialCapacity) : data(nullptr), m_size(0), m_capacity(0) {
        if (initialCapacity < 1) initialCapacity = 10;
        reallocate(initialCapacity);
    }

    // 3. Copy Constructor (Deep Copy)
    Vector(const Vector<T>& other) : data(nullptr), m_size(other.m_size), m_capacity(other.m_capacity) {
        if (m_capacity > 0) {
            data = new T[m_capacity];
            for (size_t i = 0; i < m_size; ++i) {
                data[i] = other.data[i];
            }
        }
    }

    // 4. Move Constructor (C++11) - Chuyển giao quyền sở hữu vùng nhớ
    Vector(Vector<T>&& other) noexcept : data(other.data), m_size(other.m_size), m_capacity(other.m_capacity) {
        other.data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    // 5. Copy Assignment Operator
    Vector<T>& operator=(const Vector<T>& other) {
        if (this != &other) {
            delete[] data;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            data = nullptr;
            if (m_capacity > 0) {
                data = new T[m_capacity];
                for (size_t i = 0; i < m_size; ++i) {
                    data[i] = other.data[i];
                }
            }
        }
        return *this;
    }

    // 6. Move Assignment Operator
    Vector<T>& operator=(Vector<T>&& other) noexcept {
        if (this != &other) {
            delete[] data;
            data = other.data;
            m_size = other.m_size;
            m_capacity = other.m_capacity;

            other.data = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }
        return *this;
    }

    // 7. Destructor - Giải phóng an toàn bộ nhớ động
    ~Vector() {
        delete[] data;
        data = nullptr;
        m_size = 0;
        m_capacity = 0;
    }

    // Lấy số lượng phần tử thực tế
    size_t size() const {
        return m_size;
    }

    // Lấy dung lượng tối đa hiện tại
    size_t capacity() const {
        return m_capacity;
    }

    // Kiểm tra danh sách có rỗng không
    bool empty() const {
        return m_size == 0;
    }

    // Thêm phần tử vào cuối
    void push_back(const T& element) {
        if (m_size >= m_capacity) {
            reallocate(m_capacity == 0 ? 10 : m_capacity * 2);
        }
        data[m_size++] = element;
    }

    // Thêm phần tử chuyển giao (Move push_back)
    void push_back(T&& element) {
        if (m_size >= m_capacity) {
            reallocate(m_capacity == 0 ? 10 : m_capacity * 2);
        }
        data[m_size++] = std::move(element);
    }

    // Xóa phần tử cuối cùng
    void pop_back() {
        if (m_size > 0) {
            m_size--;
        }
    }

    // Xóa phần tử tại vị trí index
    void erase(size_t index) {
        if (index >= m_size) {
            throw std::out_of_range("Chi so xoa nam ngoai pham vi hop le!");
        }
        for (size_t i = index; i + 1 < m_size; ++i) {
            data[i] = std::move(data[i + 1]);
        }
        m_size--;
    }

    // Truy cập phần tử bằng toán tử []
    T& operator[](size_t index) {
        return data[index];
    }

    const T& operator[](size_t index) const {
        return data[index];
    }

    // Truy cập phần tử có kiểm tra biên an toàn (at)
    T& at(size_t index) {
        if (index >= m_size) {
            throw std::out_of_range("Chi so vuot qua gioi han cua Vector!");
        }
        return data[index];
    }

    const T& at(size_t index) const {
        if (index >= m_size) {
            throw std::out_of_range("Chi so vuot qua gioi han cua Vector!");
        }
        return data[index];
    }

    // Xóa trắng toàn bộ phần tử
    void clear() {
        m_size = 0;
    }

    // Dự trữ dung lượng bộ nhớ
    void reserve(size_t newCapacity) {
        if (newCapacity > m_capacity) {
            reallocate(newCapacity);
        }
    }

    // Sắp xếp các phần tử bằng QuickSort nội bộ với con trỏ hàm so sánh
    void sort(bool (*compare)(const T&, const T&)) {
        if (m_size > 1) {
            quickSort(0, (int)m_size - 1, compare);
        }
    }
};

#endif // VECTOR_H
