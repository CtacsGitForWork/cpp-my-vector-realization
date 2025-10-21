#pragma once

#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <optional>
#include <algorithm>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory&) = delete;

    RawMemory(RawMemory&& other) noexcept
        : buffer_(other.buffer_)
        , capacity_(other.capacity_) {
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }
    
    RawMemory& operator=(RawMemory&& other) noexcept {
        if (this != &other) {
            Deallocate(buffer_);  // Освобождаем текущие ресурсы
        
            buffer_ = other.buffer_;
            capacity_ = other.capacity_;
            
            other.buffer_ = nullptr;
            other.capacity_ = 0;
        }

        return *this;
    }    

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;    

    // Итераторы
    iterator begin() noexcept { 
        return data_.GetAddress(); 
    }

    iterator end() noexcept { 
        return data_.GetAddress() + size_; 
    }

    const_iterator begin() const noexcept { 
        return data_.GetAddress(); 
    }

    const_iterator end() const noexcept { 
        return data_.GetAddress() + size_; 
    }

    const_iterator cbegin() const noexcept { 
        return begin(); 
    }

    const_iterator cend() const noexcept { 
        return end(); 
    }

    // Конструкторы
    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(begin(), size);
    }

    // Конструктор копирования
    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.begin(), size_, begin());
    }

    // Конструктор перемещения
    Vector(Vector&& other) noexcept
        : data_(std::move(other.data_))
        , size_(other.size_) {
        other.size_ = 0;        
    }

    // Деструктор
    ~Vector() {
        std::destroy_n(begin(), size_);
    }

    // Оператор присваивания с базовой гарантией исключений
    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                // Строгая гарантия через copy-and-swap
                Vector(rhs).Swap(*this);
            } else {                
                // Перезапись общих элементов
                size_t i = 0;
                const size_t common_size = std::min(size_, rhs.size_);
                std::copy_n(rhs.begin(), common_size, begin());

                // Обработка хвоста
                if (size_ < rhs.size_) {
                    std::uninitialized_copy_n(rhs.begin() + i, rhs.size_ - size_, begin() + i);
                } else {
                    std::destroy_n(begin() + rhs.size_, size_ - rhs.size_);
                }
            }
        }

        size_ = rhs.size_;
        return *this;
    }
    
    // Оператор перемещения 
    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            this->Swap(rhs);
        }
        return *this;
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    // Методы доступа
    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    bool Empty() const noexcept {
        return size_ == 0;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    T& front() {
        assert(size_ > 0 && "Cannot call front() on empty vector");       
        return data_[0];
    }

    const T& front() const {
        assert(size_ > 0 && "Cannot call front() on empty vector");
        return data_[0];
    }

    T& back() {
        assert(size_ > 0 && "Cannot call back() on empty vector"); 
        return data_[size_ - 1];
    }

    const T& back() const {
        assert(size_ > 0 && "Cannot call back() on empty vector");
        return data_[size_ - 1];
    }

    T& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Vector::At: index out of range");
        }
        return data_[index];
    }

    const T& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Vector::At: index out of range");
        }
        return data_[index];
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }

        RawMemory<T> new_data(new_capacity);
       
        MoveOrCopyRange(begin(), end(), new_data.GetAddress());

        std::destroy_n(begin(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size) {
        if (new_size < size_) {            
            std::destroy_n(begin() + new_size, size_ - new_size);
        } else if (new_size > size_) {            
            if (new_size > data_.Capacity()) {
                Reserve(new_size);
            }           
            
            std::uninitialized_value_construct_n(end(), new_size - size_);
        }
        size_ = new_size;
    }

    // Очистка содержимого вектора без освобождения памяти
    void Clear() noexcept {
        std::destroy_n(begin(), size_);
        size_ = 0;
    }

    //Методы размещения и удаления
    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        return *Emplace(end(), std::forward<Args>(args)...);
    }

    void PushBack(const T& value) {
        EmplaceBack(value);
    }

    void PushBack(T&& value) {
        EmplaceBack(std::move(value));
    }

    void PopBack() noexcept {
        assert(size_ > 0 && "PopBack() called on empty vector");        
        --size_;
        std::destroy_at(data_ + size_);
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        assert(pos >= begin() && pos <= end() && "Invalid position for Emplace");
        size_t index = pos - begin();

        if (size_ == Capacity()) {
            // Реаллокация
            EmplaceWithReallocation(index, std::forward<Args>(args)...);
        } else {
            // Без реаллокации
            EmplaceWithoutReallocation(index, std::forward<Args>(args)...);        
        }

        ++size_;
        return begin() + index;
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }


    iterator Erase(const_iterator pos) {
        assert(pos >= begin() && pos < end() && "Invalid position for Erase");        
        
        iterator mutable_pos = begin() + (pos - begin());        
        std::destroy_at(mutable_pos);
        
        // Сдвигаем последующие элементы влево на одну поз.
        if (mutable_pos + 1 != end()) {
            std::move(mutable_pos + 1, end(), mutable_pos);
        }     
       
        --size_;
                
        return mutable_pos;
    }

private:  
    // Выбираем перемещение, если оно noexcept, иначе копирование.
    void MoveOrCopyRange(T* from_begin, T* from_end, T* to_begin) {
        size_t count = from_end - from_begin;
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(from_begin, count, to_begin);
        } else {
            std::uninitialized_copy_n(from_begin, count, to_begin);
        }
    }

    template <typename... Args>
    void EmplaceWithReallocation(size_t index, Args&&... args) {
        const size_t new_capacity = Capacity() == 0 ? 1 : Capacity() * 2;
        RawMemory<T> new_data(new_capacity);

        // Конструируем новый элемент на новом месте
        T* new_element = new (new_data + index) T(std::forward<Args>(args)...);

        // Перемещаем/копируем  элементы ДО `index` с диагностикой
        try {
            MoveOrCopyRange(begin(), begin() + index, new_data.GetAddress());
        } catch (const std::exception& e) {
            std::destroy_at(new_element);
            throw std::runtime_error("Failed to move elements BEFORE insertion: " + std::string(e.what()));
        } catch (...) {
            std::destroy_at(new_element);
            throw std::runtime_error("Unknown error while moving elements BEFORE insertion");
        }

        // Перемещаем/копируем элементы ПОСЛЕ `index` с диагностикой
        try {
            MoveOrCopyRange(begin() + index, end(), new_data.GetAddress() + index + 1);
        } catch (const std::exception& e) {
            std::destroy_n(new_data.GetAddress(), index + 1);
            throw std::runtime_error("Failed to move elements AFTER insertion: " + std::string(e.what()));
        } catch (...) {
            std::destroy_n(new_data.GetAddress(), index + 1);
            throw std::runtime_error("Unknown error while moving elements AFTER insertion");
        }
        
        std::destroy_n(begin(), size_);
        data_.Swap(new_data);        
    }

    template <typename... Args>
    void EmplaceWithoutReallocation(size_t index, Args&&... args) {
        if (index == size_) {
            // Вставка в конец
            new (data_ + size_) T(std::forward<Args>(args)...);
        } else {
            // Вставка в середину с временным элементом
            // Создаем временный элемент
            T temp(std::forward<Args>(args)...);
            // Сдвигаем хвост с конца на одну позицию вперёд
            new (data_ + size_) T(std::move(data_[size_ - 1]));

            try {
                // Сдвигаем на одну поз. вправо остальные элементы с конца до index
                std::move_backward(begin() + index, end() - 1, end());
                // Заменяем элемент на позиции index
                data_[index] = std::move(temp);
            } catch (...) {
                std::destroy_at(data_ + size_);
                throw;
            }
        }
    }

    RawMemory<T> data_;
    size_t size_ = 0;
};
