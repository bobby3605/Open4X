#ifndef SUB_ALLOCATOR_H_
#define SUB_ALLOCATOR_H_
#include "../Vulkan/common.hpp"
#include "base_allocator.hpp"
#include <mutex>
#include <stack>
#include <type_traits>

#define is_sub_allocator std::is_same<typename AllocatorT::AllocT, SubAllocation>::value
#define is_base_allocator std::is_base_of<BaseAllocator<typename AllocatorT::AllocT>, AllocatorT>::value
// Needed to ensure at compile time that one of the if constexpr branches will be taken
#define is_allocator is_sub_allocator || is_base_allocator

template <typename AllocatorT>
    requires(is_allocator)
class SubAllocator : public Allocator<SubAllocation, typename AllocatorT::AllocT> {
  public:
    SubAllocator(size_t const& byte_size, AllocatorT* parent_allocator) : _parent_allocator(parent_allocator) {
        if constexpr (is_sub_allocator) {
            this->_base_alloc = _parent_allocator->alloc(byte_size);
        } else if constexpr (is_base_allocator) {
            // NOTE:
            // get _base_alloc from parent_allocator if parent_allocator is a BaseAllocator
            this->_base_alloc = _parent_allocator->base_alloc();
        }
    }
    ~SubAllocator() {
        if constexpr (is_sub_allocator) {
            // NOTE: Only free if parent_allocator is a SubAllocator
            _parent_allocator->free(this->_base_alloc);
        }
    }

    void write(SubAllocation const& dst_allocation, const void* data, size_t const& byte_size) {
        std::lock_guard<std::mutex> lock(this->_realloc_lock);
        if constexpr (is_sub_allocator) {
            SubAllocation dst = dst_allocation;
            dst.offset += this->_base_alloc.offset;
            _parent_allocator->write(dst, data, byte_size);
        } else if constexpr (is_base_allocator) {
            _parent_allocator->write(dst_allocation, data, byte_size);
        }
    }
    void copy(SubAllocation const& dst_allocation, SubAllocation const& src_allocation, size_t const& copy_size) {
        if constexpr (is_sub_allocator) {
            // TODO:
            // realloc lock here
            // can't do it now because realloc calls that lock and it calls this function;
            SubAllocation dst = dst_allocation;
            dst.offset += this->_base_alloc.offset;
            SubAllocation src = src_allocation;
            src.offset += this->_base_alloc.offset;
            _parent_allocator->copy(dst, src, copy_size);
        } else if constexpr (is_base_allocator) {

            _parent_allocator->copy(dst_allocation, src_allocation, copy_size);
        }
    }
    void get(void* dst, SubAllocation const& src_allocation, size_t const& byte_size) {
        if constexpr (is_sub_allocator) {
            SubAllocation src = src_allocation;
            src.offset += this->_base_alloc.offset;
            _parent_allocator->get(dst, src, byte_size);
        } else if constexpr (is_base_allocator) {
            _parent_allocator->get(dst, src_allocation, byte_size);
        }
    }
    const AllocatorT* parent() const { return _parent_allocator; }

  protected:
    AllocatorT* _parent_allocator;
    void realloc_check_parent(size_t const& byte_size) {
        _parent_allocator->realloc(this->_base_alloc, byte_size);
        // NOTE:
        // Ensure that the base allocator for a BaseAllocator is updated whenever there's a reallocation
        // This is needed for functions like write()
        if constexpr (is_base_allocator) {
            _parent_allocator->set_base_alloc(this->_base_alloc);
        }
    }
};

template <typename AllocatorT> class StackAllocator : public SubAllocator<AllocatorT> {
  public:
    StackAllocator(size_t const& block_size, AllocatorT* parent_allocator)
        : SubAllocator<AllocatorT>(block_size, parent_allocator), _block_size(block_size) {
        // SubAllocator constructor makes a buffer with block_size total byte size
        _block_count = 1;
        // _free_blocks is a stack of indices to blocks allocated from a base allocator
        // NOTE:
        // stack instead of queue so that the last freed blocks will be used first,
        // this is better for memory locality
        // _free_blocks is created in reverse order so that the base offset will be used first
        // NOTE:
        // can't get c++ 23 ranges to work correctly
        for (int i = _block_count - 1; i >= 0; --i) {
            _free_blocks.push(i);
        }
    }

    // NOTE:
    // byte_size does nothing,
    // but this is needed to get the virtual function to compile
    // There's probably a better way to do this
    SubAllocation alloc(size_t const& byte_size = 0, size_t const& alignment = 1) {
        if (_free_blocks.empty()) {
            // Out of memory error
            // realloc
            size_t base_size = this->_base_alloc.size;
            size_t new_block_index = ++_block_count;
            size_t new_parent_size = base_size + _block_size;
            this->realloc_check_parent(new_parent_size);
            _free_blocks.push(new_block_index);
        }
        SubAllocation allocation;
        allocation.size = _block_size;
        // offset is the block index * the block size
        allocation.offset = _block_size * _free_blocks.top();
        _free_blocks.pop();
        return allocation;
    }

    void free(SubAllocation const& alloc) { _free_blocks.push(alloc.offset / _block_size); }
    size_t const& block_size() const { return _block_size; }
    int const& block_count() const { return _block_count; }

  private:
    size_t _block_size;
    int _block_count;
    std::stack<size_t> _free_blocks;
};

template <typename AllocatorT> class LinearAllocator : public SubAllocator<AllocatorT> {
  private:
    std::vector<SubAllocation> _free_blocks;

  public:
    LinearAllocator(AllocatorT* parent_allocator) : SubAllocator<AllocatorT>(0, parent_allocator) {
        SubAllocation allocation{};
        allocation.offset = 0;
        allocation.size = this->_base_alloc.size;
        _free_blocks.emplace_back(allocation);
    }

    // TODO
    // explore moving alignment into constructor
    SubAllocation alloc(size_t const& byte_size, size_t const& alignment = 1) {
        SubAllocation allocation;
        allocation.size = align(byte_size, alignment);
        for (auto free_block = _free_blocks.begin(); free_block != _free_blocks.end(); ++free_block) {
            // If block has enough size,
            // shrink it
            // return the sub allocation
            if (free_block->size > allocation.size) {
                allocation.offset = free_block->offset;
                free_block->offset += allocation.size;
                free_block->size -= allocation.size;
                return allocation;
            }
            // If blocks are equal,
            // remove the block from the free blocks
            // return the free block
            if (free_block->size == allocation.size) {
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
        // TODO:
        // Expand free block for any extra memory created by alignment
        // This isn't right. I need to expand the last block in an _created_blocks vector
        // _free_blocks.end()->size(align(this->_base_alloc.size(), alignment));
        // NOTE:
        // adding alignment here to ensure that the last block doesn't get overwritten
        allocation.offset = align(this->_base_alloc.size, alignment) + alignment;
        size_t new_size = allocation.offset + allocation.size;
        this->realloc_check_parent(new_size);
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
