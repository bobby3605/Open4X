#ifndef SUB_ALLOCATOR_H_
#define SUB_ALLOCATOR_H_
#include "allocation.hpp"
#include <cstdint>
#include <deque>
#include <queue>
#include <stdexcept>

template <typename ParentAllocationT> class VoidAllocator {
  public:
    SubAllocation<VoidAllocator, ParentAllocationT>* alloc(size_t const& byte_size) {
        throw std::runtime_error("unimplemented alloc(byte_size)");
    }
};

template <typename ParentAllocationT> class LinearAllocator {
    ParentAllocationT* _parent;
    std::vector<SubAllocation<LinearAllocator, ParentAllocationT>*> _free_blocks;

  public:
    // TODO
    // explore putting alignment into constructor
    LinearAllocator(ParentAllocationT* parent) : _parent(parent) {}
    const ParentAllocationT* parent() { return _parent; }

    SubAllocation<LinearAllocator, ParentAllocationT>* alloc(size_t const& byte_size) {
        for (size_t i = 0; i < _free_blocks.size(); ++i) {
            auto free_block = _free_blocks[i];
            // If block has enough size,
            // shrink it
            // return the sub allocation
            if (free_block->_size > byte_size) {
                free_block->_offset += byte_size;
                free_block->_size -= byte_size;
                return new SubAllocation<LinearAllocator, ParentAllocationT>(free_block->_offset, byte_size, this, _parent);
            }
            // If blocks are equal,
            // remove the block from the free blocks
            // return the free block
            if (free_block->_size == byte_size) {
                SubAllocation<LinearAllocator, ParentAllocationT>* block = free_block;
                _free_blocks.erase(_free_blocks.begin() + i);
                return block;
            }
        }
        // No free block found,
        // out of memory error
        // allocate a new block and return it
        SubAllocation<LinearAllocator, ParentAllocationT>* block =
            new SubAllocation<LinearAllocator, ParentAllocationT>(_parent->size(), byte_size, this, _parent);
        _parent->realloc(block->_offset + byte_size);
        return block;
    }

    // NOTE:
    // This can be a reference because push_back will copy it
    void free(SubAllocation<LinearAllocator, ParentAllocationT>* alloc) {
        // TODO
        // Auto sort and combine free blocks
        _free_blocks.push_back(alloc);
    }
};

template <typename ParentAllocationT> class FixedAllocator {
    ParentAllocationT* _parent;
    size_t _block_size;
    size_t _block_count = 0;
    std::queue<SubAllocation<FixedAllocator, ParentAllocationT>*> _free_blocks;

  public:
    FixedAllocator(size_t const& block_size, ParentAllocationT* parent) : _parent(parent), _block_size(block_size) {}
    const ParentAllocationT* parent() const { return _parent; }
    size_t const& block_size() const { return _block_size; }
    size_t const& block_count() const { return _block_count; }

    SubAllocation<FixedAllocator, ParentAllocationT>* alloc(size_t const& byte_size) {
        throw std::runtime_error("unimplemented alloc(byte_size)");
    }

    // TODO:
    // destructor
    SubAllocation<FixedAllocator, ParentAllocationT>* alloc() {
        if (_free_blocks.empty()) {
            // alloc new block if empty
            grow(1);
        }
        SubAllocation<FixedAllocator, ParentAllocationT>* output = _free_blocks.front();
        _free_blocks.pop();
        return output;
    }
    void free(SubAllocation<FixedAllocator, ParentAllocationT>* allocation) { _free_blocks.push(allocation); }

  private:
    void grow(uint32_t count) {
        size_t base_size = _parent->size();
        _parent->realloc(base_size + _block_size * count);
        for (uint32_t i = 0; i < count; ++i) {
            _free_blocks.push(
                new SubAllocation<FixedAllocator, ParentAllocationT>(base_size + _block_size * i, _block_size, this, _parent));
            ++_block_count;
        }
    }
};

// TODO:
// Write unit tests for this
template <typename ParentAllocationT> class ContiguousFixedAllocator {
    ParentAllocationT* _parent;
    size_t _block_size;
    size_t _block_count = 0;
    std::deque<SubAllocation<ContiguousFixedAllocator, ParentAllocationT>*> _free_blocks;
    std::deque<SubAllocation<ContiguousFixedAllocator, ParentAllocationT>*> _alloced_blocks;

  public:
    ContiguousFixedAllocator(size_t const& block_size, ParentAllocationT* parent) : _parent(parent), _block_size(block_size) {}
    const ParentAllocationT* parent() const { return _parent; }
    size_t const& block_size() const { return _block_size; }
    size_t const& block_count() const { return _block_count; }
    SubAllocation<ContiguousFixedAllocator, ParentAllocationT>* alloc(size_t const& byte_size) {
        throw std::runtime_error("unimplemented alloc(byte_size)");
    }

    // TODO:
    // destructor
    SubAllocation<ContiguousFixedAllocator, ParentAllocationT>* alloc() {
        if (_free_blocks.empty()) {
            // alloc new block if empty
            grow(1);
        }
        SubAllocation<ContiguousFixedAllocator, ParentAllocationT>* output = *_free_blocks.begin();
        // output = 0
        _free_blocks.pop_front();
        // _free_blocks = [1, 2]
        _alloced_blocks.push_back(output);
        // _alloced_blocks = [0]
        return output;
        // alloc again
        // _alloced_blocks = [0, 1]
        // _free_blocks = [2]
    }
    void pop_and_swap(SubAllocation<ContiguousFixedAllocator, ParentAllocationT>* allocation) {
        // after alloc twice
        // suppose allocation = 0
        SubAllocation<ContiguousFixedAllocator, ParentAllocationT>* last_block = *_alloced_blocks.end();
        // last_block = 1
        _alloced_blocks.pop_back();
        // _alloced_blocks = [0]
        // copy last block to allocation
        last_block->copy(allocation);
        size_t last_offset = last_block->offset();
        // set last block offset to allocation offset
        // Because last_block is a pointer, this updates the offset for whatever owns the allocation
        // last_block is now whatever allocation was
        last_block->_offset = allocation->offset();
        // last_block = 0
        // Set allocation's offset to the last offset,
        // allocation = 1
        allocation->_offset = last_offset;

        // Push the last block to the free blocks
        // allocation needs to be pushed here, because it is free,
        // while last_block is still owned by something
        _free_blocks.push_front(allocation);
        // _free_blocks = [1, 2]
    }

  private:
    void grow(uint32_t count) {
        size_t base_size = _parent->size();
        _parent->realloc(base_size + _block_size * count);
        for (uint32_t i = 0; i < count; ++i) {
            _free_blocks.push_back(
                new SubAllocation<ContiguousFixedAllocator, ParentAllocationT>(base_size + _block_size * i, _block_size, this, _parent));
            // _free_blocks = [0,1,2]
            ++_block_count;
        }
    }
};

#endif // SUB_ALLOCATOR_H_
