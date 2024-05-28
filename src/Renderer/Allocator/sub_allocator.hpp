#ifndef SUB_ALLOCATOR_H_
#define SUB_ALLOCATOR_H_
#include "base_allocator.hpp"
#include <stack>
#include <type_traits>

// FIXME:
// It shouldn't be Allocator<SubAllocation> It should be Allocator<ParentAllocationT>
template <typename AllocatorT> class SubAllocator : public Allocator<SubAllocation> {
    // FIXME:
    // Static assert for more than just GPU
    // Need to recursively check the allocator types
    static_assert(std::is_base_of<Allocator, AllocatorT>::value | std::is_base_of<BaseAllocator<GPUAllocation>, AllocatorT>::value,
                  "AllocatorT must be an Allocator");

  public:
    SubAllocator(size_t const& byte_size, AllocatorT* parent_allocator) : _parent_allocator(parent_allocator) {
        // If a SubAllocator is the parent, then allocate from it
        // If a BaseAllocator is the parent, set _base_alloc to the parent _base_alloc
        if (std::is_base_of<SubAllocator, AllocatorT>::value) {
            _base_alloc = _parent_allocator->alloc(byte_size);
        } else {
            _base_alloc = _parent_allocator->base_alloc();
        }
    }
    void write(SubAllocation const& dst_allocation, const void* data, size_t const& byte_size) {
        _parent_allocator->write(dst_allocation, data, byte_size);
    }
    void copy(SubAllocation const& dst_allocation, SubAllocation const& src_allocation, size_t const& copy_size) {
        _parent_allocator->copy(dst_allocation, src_allocation, copy_size);
    }

  protected:
    AllocatorT* _parent_allocator;
    // void realloc(size_t const& byte_size);
};

template <typename AllocatorT> class StackAllocator : public SubAllocator<AllocatorT> {
  public:
    StackAllocator(size_t const& block_size, AllocatorT* base_allocator)
        : SubAllocator<AllocatorT>(block_size, base_allocator), _block_size(block_size) {
        size_t block_count = SubAllocator<AllocatorT>::_base_alloc.size() / block_size;
        // _free_blocks is a stack of indices to blocks allocated from a base allocator
        // NOTE:
        // stack instead of queue so that the last freed blocks will be used first,
        // this is better for memory locality
        // _free_blocks is created in reverse order so that the base offset will be used first
        // NOTE:
        // can't get c++ 23 ranges to work correctly
        for (size_t i = block_count - 1; i >= 0; --i) {
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
            size_t base_size = SubAllocator<AllocatorT>::_base_alloc.size();
            size_t total_block_count = base_size / _block_size;
            size_t new_block_index = total_block_count + 1;
            size_t new_parent_size = base_size + _block_size;
            // NOTE:
            // If _parent_allocator is a BaseAllocator, then _parent_allocator->_base_alloc doesn't get updated.
            // This is fine, because if _parent_allocator is a BaseAllocator, then _parent_allocator->_base_alloc isn't used after the
            // constructor. Ideally, if _parent_allocator is a BaseAllocator, then SubAllocator::_base_alloc would be a reference
            // This would require tracking all allocated blocks in an immutable manner
            // Tracking blocks that way might be needed for updating indices anyway
            SubAllocator<AllocatorT>::_parent_allocator->realloc(SubAllocator<AllocatorT>::_base_alloc, new_parent_size);
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

  private:
    size_t _block_size;
    std::stack<size_t> _free_blocks;
};

#endif // SUB_ALLOCATOR_H_
