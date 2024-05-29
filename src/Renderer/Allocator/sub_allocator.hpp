#ifndef SUB_ALLOCATOR_H_
#define SUB_ALLOCATOR_H_
#include "base_allocator.hpp"
#include <mutex>
#include <stack>
#include <type_traits>

template <typename AllocatorT> class SubAllocator : public Allocator<SubAllocation, typename AllocatorT::AllocT> {

  public:
    SubAllocator(size_t const& byte_size, AllocatorT* parent_allocator)
        // FIXME:
        // SubAllocator<GPUAllocator> needs to work for any SubAllocator<T>
        requires(std::is_base_of<SubAllocator<GPUAllocator>, AllocatorT>::value)
        : _parent_allocator(parent_allocator) {
        this->_base_alloc = _parent_allocator->alloc(byte_size);
    }
    // FIXME:
    // Make this work for CPU and GPU, not just GPU
    SubAllocator(size_t const& byte_size, AllocatorT* parent_allocator)
        requires(std::is_base_of<BaseAllocator<GPUAllocation>, AllocatorT>::value)
        : _parent_allocator(parent_allocator) {
        this->_base_alloc = _parent_allocator->base_alloc();
    }
    ~SubAllocator()
        requires(std::is_base_of<SubAllocator<typename AllocatorT::BaseT>, AllocatorT>::value)
    {
        _parent_allocator->free(this->_base_alloc);
    }
    ~SubAllocator() {}

    void write(SubAllocation const& dst_allocation, const void* data, size_t const& byte_size) {
        std::lock_guard<std::mutex> lock(this->_realloc_lock);
        // FIXME:
        // This isn't right
        // I need to add _base_alloc to this or something like that
        _parent_allocator->write(dst_allocation, data, byte_size);
    }
    void copy(SubAllocation const& dst_allocation, SubAllocation const& src_allocation, size_t const& copy_size) {
        _parent_allocator->copy(dst_allocation, src_allocation, copy_size);
    }

    const AllocatorT* parent() const { return _parent_allocator; }

  protected:
    AllocatorT* _parent_allocator;
};

template <typename AllocatorT> class StackAllocator : public SubAllocator<AllocatorT> {
  public:
    StackAllocator(size_t const& block_size, AllocatorT* parent_allocator)
        : SubAllocator<AllocatorT>(block_size, parent_allocator), _block_size(block_size) {
        _block_count = this->_base_alloc.size() / block_size;
        // _free_blocks is a stack of indices to blocks allocated from a base allocator
        // NOTE:
        // stack instead of queue so that the last freed blocks will be used first,
        // this is better for memory locality
        // _free_blocks is created in reverse order so that the base offset will be used first
        // NOTE:
        // can't get c++ 23 ranges to work correctly
        for (size_t i = _block_count - 1; i >= 0; --i) {
            _free_blocks.push(i);
        }
    }

    // NOTE:
    // byte_size does nothing,
    // but this is needed to get the virtual function to compile
    // There's probably a better way to do this
    SubAllocation alloc(size_t const& byte_size = 0) {
        if (_free_blocks.empty()) {
            // Out of memory error
            // realloc
            size_t base_size = this->_base_alloc.size();
            size_t new_block_index = ++_block_count;
            size_t new_parent_size = base_size + _block_size;
            // NOTE:
            // This doesn't set BaseAllocation::_base_alloc
            this->_parent_allocator->realloc(this->_base_alloc, new_parent_size);
            _free_blocks.push(new_block_index);
        }
        SubAllocation allocation;
        allocation.size_ = _block_size;
        // offset is the block index * the block size
        allocation.offset = _block_size * _free_blocks.top();
        _free_blocks.pop();
        return allocation;
    }

    void free(SubAllocation const& alloc) { _free_blocks.push(alloc.offset / _block_size); }
    size_t const& block_size() const { return _block_size; }
    size_t const& block_count() const { return _block_count; }

  private:
    size_t _block_size;
    size_t _block_count;
    std::stack<size_t> _free_blocks;
};

template <typename AllocatorT> class LinearAllocator : public SubAllocator<AllocatorT> {
  private:
    std::vector<SubAllocation> _free_blocks;

  public:
    LinearAllocator(AllocatorT* parent_allocator) : SubAllocator<AllocatorT>(0, parent_allocator) {}

    SubAllocation alloc(size_t const& byte_size) {
        SubAllocation allocation;
        allocation.size_ = byte_size;
        for (auto free_block = _free_blocks.begin(); free_block != _free_blocks.end(); ++free_block) {
            // If block has enough size,
            // shrink it
            // return the sub allocation
            if (free_block->size() > allocation.size()) {
                allocation.offset = free_block->offset;
                free_block->offset += allocation.size();
                free_block->size_ -= allocation.size();
                return allocation;
            }
            // If blocks are equal,
            // remove the block from the free blocks
            // return the free block
            if (free_block->size() == allocation.size()) {
                allocation.offset = free_block->offset;
                // TODO
                // Replace this with a vector made from Allocator
                // Or a better method of handling free blocks
                _free_blocks.erase(free_block);
                return allocation;
            }
        }
        // No free block found,
        // out of memory error
        // allocate a new block and return it
        allocation.offset = this->_base_alloc.size();
        this->_parent_allocator->realloc(this->_base_alloc, allocation.offset + allocation.size());
        return allocation;
    }

    // NOTE:
    // This can be a reference because push_back will copy it
    void free(SubAllocation const& alloc) {
        // TODO
        // Auto sort and combine free blocks
        _free_blocks.push_back(alloc);
    }
};

#endif // SUB_ALLOCATOR_H_
