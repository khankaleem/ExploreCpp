#include<new>
#include<vector>
#include<stdexcept>
#include<cassert>
#include <memory>
#include <utility>
#include<iostream>

template<typename ElemT>
class Deque {
private:
    static constexpr size_t BLOCK_SIZE = 1<<8;
    struct Block {
        struct Deleter {
            void operator()(ElemT* ptr_) const noexcept {
                ::operator delete(ptr_, std::align_val_t(alignof(ElemT)));
            }
        };
        std::unique_ptr<ElemT, Deleter> m_data;
        explicit Block() : m_data{reinterpret_cast<ElemT*>(
            ::operator new(sizeof(ElemT) * BLOCK_SIZE, std::align_val_t(alignof(ElemT))) )} {
        }
        ElemT& operator[](size_t i_) {
            return *(m_data.get() + i_);
        }
        const ElemT& operator[](size_t i_) const {
            return *(m_data.get() + i_);
        }
        void constructAt(size_t i_, const ElemT& val_) {
            new (m_data.get()+i_) ElemT(val_);
        }
        void constructAt(size_t i_, ElemT&& val_) {
            new (m_data.get()+i_) ElemT(std::move(val_));
        }
        template<typename... ArgsT>
        void emplaceAt(size_t i_, ArgsT&&... val_) {
            new (m_data.get()+i_) ElemT(std::forward<ArgsT>(val_)...);
        }
        void destroyAt(size_t i_) {
            (m_data.get()+i_)->~ElemT();
        }
    };
    std::vector<std::unique_ptr<Block>> m_blockPtrs; // Use array of pointers instead with starting slot in the middle to eliminate push front cost
    size_t m_frontOffset{}; // Offset in Block where Deque's first element is present
    size_t m_backOffset{}; // Offset in block where Deque's next element is to be inserted
    size_t m_size{};

public:
    Deque() = default;


    Deque(const Deque&) = delete;
    Deque& operator=(const Deque&) = delete;
    ~Deque() {
        for (size_t i = 0; i < m_blockPtrs.size(); ++i) {
            size_t start = (i == 0) ? m_frontOffset : 0;
            size_t end = (i == m_blockPtrs.size() - 1) ? m_backOffset : BLOCK_SIZE;
            for (size_t j = start; j < end; ++j) {
                m_blockPtrs[i]->destroyAt(j);
            }
        }
    }

    //pushfront
    void pushFront(const ElemT& val_) {
        if (m_size == 0 || m_frontOffset == 0) {
            auto block = std::make_unique<Block>();
            block->constructAt(BLOCK_SIZE-1, val_);
            m_frontOffset = BLOCK_SIZE-1;
            m_backOffset = (m_size == 0 ? BLOCK_SIZE : m_backOffset);
            m_blockPtrs.insert(m_blockPtrs.begin(), std::move(block));
            m_size++;
        }
        else {
            m_blockPtrs.front()->constructAt(m_frontOffset-1, val_);
            m_frontOffset--;
            m_size++;
        }
    }
    void pushFront(ElemT&& val_) {
        if (m_size == 0 || m_frontOffset == 0) {
            auto block = std::make_unique<Block>();
            block->constructAt(BLOCK_SIZE-1, std::move(val_));
            m_frontOffset = BLOCK_SIZE-1;
            m_backOffset = (m_size == 0 ? BLOCK_SIZE : m_backOffset);
            m_blockPtrs.insert(m_blockPtrs.begin(), std::move(block));
            m_size++;
        }
        else {
            m_blockPtrs.front()->constructAt(m_frontOffset-1, std::move(val_));
            m_frontOffset--;
            m_size++;
        }
    }
    template<typename... ArgsT>
    void emplaceFront(ArgsT&&... args_) {
        if (m_size == 0 || m_frontOffset == 0) {
            auto block = std::make_unique<Block>();
            block->emplaceAt(BLOCK_SIZE-1, std::forward<ArgsT>(args_)...);
            m_frontOffset = BLOCK_SIZE-1;
            m_backOffset = (m_size == 0 ? BLOCK_SIZE : m_backOffset);
            m_blockPtrs.insert(m_blockPtrs.begin(), std::move(block));
            m_size++;
        }
        else {
            m_blockPtrs.front()->emplaceAt(m_frontOffset-1, std::forward<ArgsT>(args_)...);
            m_frontOffset--;
            m_size++;
        }
    }
    void popFront() {
        m_blockPtrs.front()->destroyAt(m_frontOffset);
        m_size--;
        m_frontOffset++;
        if (m_size == 0 || m_frontOffset == BLOCK_SIZE) {
            m_blockPtrs.erase(m_blockPtrs.begin());
            m_frontOffset = 0;
        }
    }

    void pushBack(const ElemT& val_) {
        if (m_size == 0 || m_backOffset == BLOCK_SIZE) {
            auto block = std::make_unique<Block>();
            block->constructAt(0, val_);
            m_blockPtrs.insert(m_blockPtrs.end(), std::move(block));
            m_backOffset = 1;
            m_frontOffset = (m_size == 0 ? 0 : m_frontOffset);
            m_size++;
        }
        else {
            m_blockPtrs.back()->constructAt(m_backOffset, val_);
            m_backOffset++;
            m_size++;
        }
    }
    void pushBack(ElemT&& val_) {
        if (m_size == 0 || m_backOffset == BLOCK_SIZE) {
            auto block = std::make_unique<Block>();
            block->constructAt(0, std::move(val_));
            m_blockPtrs.insert(m_blockPtrs.end(), std::move(block));
            m_backOffset = 1;
            m_frontOffset = (m_size == 0 ? 0 : m_frontOffset);
            m_size++;
        }
        else {
            m_blockPtrs.back()->constructAt(m_backOffset, std::move(val_));
            m_backOffset++;
            m_size++;
        }
    }
    template<typename... ArgsT>
    void emplaceBack(ArgsT&&... args_) {
        if (m_size == 0 || m_backOffset == BLOCK_SIZE) {
            auto block = std::make_unique<Block>();
            block->emplaceAt(0, std::forward<ArgsT>(args_)...);
            m_blockPtrs.insert(m_blockPtrs.end(), std::move(block));
            m_backOffset = 1;
            m_frontOffset = (m_size == 0 ? 0 : m_frontOffset);
            m_size++;
        }
        else {
            m_blockPtrs.back()->emplaceAt(m_backOffset, std::forward<ArgsT>(args_)...);
            m_backOffset++;
            m_size++;
        }
    }
    void popBack() {
        m_blockPtrs.front()->destroyAt(m_backOffset-1);
        m_size--;
        m_backOffset--;
        if (m_size == 0 || m_backOffset == 0) {
            m_blockPtrs.pop_back();
            m_backOffset = BLOCK_SIZE;
        }
    }

    ElemT& front() {
        auto& block = *(m_blockPtrs.front());
        return block[m_frontOffset];
    }
    const ElemT& front() const {
        const auto& block = *(m_blockPtrs.front());
        return block[m_frontOffset];
    }
    ElemT& back() {
        auto& block = *(m_blockPtrs.back());
        return block[m_backOffset-1];
    }
    const ElemT& back() const {
        const auto& block = *(m_blockPtrs.back());
        return block[m_backOffset-1];
    }
    ElemT& operator[](size_t i) {
        size_t realPos = m_frontOffset + i;
        size_t blockIdx = realPos / BLOCK_SIZE;
        size_t innerIdx = realPos % BLOCK_SIZE;
        return (*m_blockPtrs[blockIdx])[innerIdx];
    }
    const ElemT& operator[](size_t i) const {
        size_t realPos = m_frontOffset + i;
        size_t blockIdx = realPos / BLOCK_SIZE;
        size_t innerIdx = realPos % BLOCK_SIZE;
        return (*m_blockPtrs[blockIdx])[innerIdx];
    }

    size_t size() const {
        return m_size;
    }
    bool empty() const {
        return m_size == 0;
    }
};

int main() {
    Deque<int> dq;
    int sum1 = 0;
    int sum2 = 0;
    for(int i = 0; i < 300; i++) {
        dq.pushBack(i);
        sum1 += i;
        assert(dq.back() == i);
        dq.pushFront(i);
        assert(dq.front() == i);
        sum1 += i;
    }
    for(int i = 0; i < 300; i++) {
        assert(dq.back() == 299-i);
        sum2 += dq.back();
        dq.popBack();
        sum2 += dq.front();
        assert(dq.front() == 299-i);
        dq.popFront();
    }
    assert(sum1 == sum2);
}