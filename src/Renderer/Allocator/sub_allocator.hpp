#ifndef SUB_ALLOCATOR_H_
#define SUB_ALLOCATOR_H_
#include "../../utils/utils.hpp"
#include "allocation.hpp"
#include <cstdint>
#include <deque>
#include <mutex>
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
    // Delay allocation, but still get a SubAlloction for this Allocator
    SubAllocation<LinearAllocator, ParentAllocationT>* alloc_0() {
        return new SubAllocation<LinearAllocator, ParentAllocationT>(0, 0, this, _parent);
    }

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

    void free(SubAllocation<LinearAllocator, ParentAllocationT>* alloc) {
        // TODO
        // Auto sort and combine free blocks
        _free_blocks.push_back(alloc);
    }
};

template <typename ParentAllocationT> class FixedAllocator {
    ParentAllocationT* _parent;
    size_t _block_size;
    std::atomic<size_t> _block_count = 0;
    safe_queue<SubAllocation<FixedAllocator, ParentAllocationT>*> _free_blocks;
    std::mutex _reserve_lock;

  public:
    FixedAllocator(size_t const& block_size, ParentAllocationT* parent) : _parent(parent), _block_size(block_size) {}
    const ParentAllocationT* parent() const { return _parent; }
    size_t const& block_size() const { return _block_size; }
    size_t const block_count() const { return _block_count; }
    void reserve(size_t count) {
        std::unique_lock<std::mutex> lock(_reserve_lock);
        _free_blocks.reserve(count);
        if (count > _block_count) {
            size_t base_size = _parent->size();
            _parent->realloc(base_size + _block_size * count);
            size_t count_to_add = count - _block_count;
            for (size_t i = 0; i < count_to_add; ++i) {
                _free_blocks.push(
                    new SubAllocation<FixedAllocator, ParentAllocationT>(_block_size * _block_count++, _block_size, this, _parent));
            }
        }
    }
    void grow(size_t count) { reserve(count + _block_count); }

    SubAllocation<FixedAllocator, ParentAllocationT>* alloc(size_t const& byte_size) {
        throw std::runtime_error("unimplemented alloc(byte_size)");
    }

    // TODO:
    // destructor
    SubAllocation<FixedAllocator, ParentAllocationT>* alloc() {
        if (_free_blocks.empty()) {
            // alloc new blocks if empty
            grow(1);
        }
        SubAllocation<FixedAllocator, ParentAllocationT>* output = _free_blocks.pop();
        return output;
    }
    void free(SubAllocation<FixedAllocator, ParentAllocationT>* allocation) {
        if (allocation->size() != 0) {
            _free_blocks.push(allocation);
        }
    }
};

// TODO:
// Write unit tests for this
template <typename ParentAllocationT> class ContiguousFixedAllocator {
    ParentAllocationT* _parent;
    size_t _block_size;
    std::atomic<size_t> _block_count = 0;
    safe_deque<SubAllocation<ContiguousFixedAllocator, ParentAllocationT>*> _free_blocks;
    safe_deque<SubAllocation<ContiguousFixedAllocator, ParentAllocationT>*> _alloced_blocks;
    std::mutex _reserve_lock;

  public:
    ContiguousFixedAllocator(size_t const& block_size, ParentAllocationT* parent) : _parent(parent), _block_size(block_size) {}
    const ParentAllocationT* parent() const { return _parent; }
    size_t const& block_size() const { return _block_size; }
    size_t block_count() const { return _block_count.load(); }
    SubAllocation<ContiguousFixedAllocator, ParentAllocationT>* alloc(size_t const& byte_size) {
        throw std::runtime_error("unimplemented alloc(byte_size)");
    }
    void reserve(size_t count) {
        std::unique_lock<std::mutex> lock(_reserve_lock);
        //  _free_blocks.reserve(count);
        if (count > _block_count) {
            size_t base_size = _parent->size();
            _parent->realloc(base_size + _block_size * count);
            std::cout << "contiguous fixed parent realloc with size: " << base_size + _block_size * count << std::endl;
            size_t count_to_add = count - _block_count;
            for (size_t i = 0; i < count_to_add; ++i) {
                _free_blocks.push_back(new SubAllocation<ContiguousFixedAllocator, ParentAllocationT>(_block_size * _block_count++,
                                                                                                      _block_size, this, _parent));
                // _free_blocks = [0,1,2]
            }
            std::cout << "block count after reserve: " << _block_count << std::endl;
        }
    }
    void grow(size_t count) { reserve(count + _block_count); }

    // TODO:
    // destructor
    SubAllocation<ContiguousFixedAllocator, ParentAllocationT>* alloc() {
        if (_free_blocks.empty()) {
            // alloc new block if empty
            grow(1);
        }
        SubAllocation<ContiguousFixedAllocator, ParentAllocationT>* output = _free_blocks.pop_front();
        // output = 0
        // _free_blocks = [1, 2]
        _alloced_blocks.push_back(output);
        // _alloced_blocks = [0]
        if (output == nullptr) {
            std::cout << "null output" << std::endl;
        }
        return output;
        // alloc again
        // _alloced_blocks = [0, 1]
        // _free_blocks = [2]
    }
    void pop_and_swap(SubAllocation<ContiguousFixedAllocator, ParentAllocationT>* allocation) {
        // after alloc twice
        // suppose allocation = 0
        SubAllocation<ContiguousFixedAllocator, ParentAllocationT>* last_block = _alloced_blocks.pop_back();
        // last_block = 1
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
};

#endif // SUB_ALLOCATOR_H_
