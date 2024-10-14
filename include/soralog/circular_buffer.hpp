/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <optional>
#include <vector>

#ifdef NDEBUG
#define IF_RELEASE true
#else
#define IF_RELEASE false
#endif

namespace soralog {

  template <typename T>
  class CircularBuffer final {
   public:
    using element_type = T;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    struct Node final {
      template <typename... Args>
      void init(Args &&...args) {
        new (item_) T(std::forward<Args>(args)...);
      }
      inline const T &item() const {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<const T *>(item_);
      }

      // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
      std::atomic_flag busy = ATOMIC_VAR_INIT(false);

     private:
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
      alignas(std::alignment_of_v<T>) char item_[sizeof(T)];
    };

    class NodeRef {
      Node *node = nullptr;

     public:
      NodeRef() noexcept = default;
      NodeRef(NodeRef &&) noexcept = delete;
      NodeRef(const NodeRef &) = delete;
      NodeRef &operator=(NodeRef &&) noexcept = delete;
      NodeRef &operator=(const NodeRef &) = delete;

      NodeRef(Node &node) noexcept : node(&node) {}

      ~NodeRef() noexcept(IF_RELEASE) {
        if (node) {
          node->busy.clear();
        }
      }

      const T &operator*() const noexcept(IF_RELEASE) {
        assert(node);
        return node->item();
      }

      const T *operator->() const noexcept(IF_RELEASE) {
        assert(node);
        return &(node->item());
      }

      explicit operator bool() const noexcept {
        return node != nullptr;
      }
    };

    CircularBuffer() = delete;
    CircularBuffer(CircularBuffer &&) noexcept = delete;
    CircularBuffer(const CircularBuffer &) = delete;
    ~CircularBuffer() = default;
    CircularBuffer &operator=(CircularBuffer &&) noexcept = delete;
    CircularBuffer &operator=(const CircularBuffer &) = delete;

    CircularBuffer(size_t capacity, size_t padding)
        : capacity_(capacity),
          element_size_([&] {
            const auto alignment = std::alignment_of_v<Node>;
            if (auto offset = padding % alignment) {
              padding += alignment - offset;
            }
            return sizeof(Node) + padding;
          }()),
          raw_data_(capacity_ * element_size_) {
      for (auto index = 0; index < capacity; ++index) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        new (raw_data_.data() + element_size_ * index) Node;
      }
    };

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    explicit CircularBuffer(size_t capacity) : CircularBuffer(capacity, 0) {};

    size_t capacity() const noexcept {
      while (busy_.test_and_set()) {
        continue;
      }
      auto ret = capacity_;
      busy_.clear();
      return ret;
    }

    size_t size() const noexcept {
      while (busy_.test_and_set()) {
        continue;
      }
      auto ret = size_.load();
      busy_.clear();
      return ret;
    }

    size_t avail() const noexcept {
      while (busy_.test_and_set()) {
        continue;
      }
      auto ret = capacity_ - size_;
      busy_.clear();
      return ret;
    }

    template <typename... Args>
    [[nodiscard]] NodeRef put(Args &&...args) noexcept(IF_RELEASE) {
      while (true) {
        if (busy_.test_and_set()) {
          continue;
        }

        // Tail is caught up - queue is full
        if (pop_index_ == push_index_ and size_ != 0) {
          busy_.clear();
          return {};
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto &node = *reinterpret_cast<Node *>(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            raw_data_.data() + element_size_ * push_index_);

        // Capture node if not busy
        if (node.busy.test_and_set()) {
          busy_.clear();
          continue;
        }

        // Go to next item place
        push_index_ = (push_index_ + 1) % capacity_;

        assert(size_ < capacity_);
        ++size_;

        busy_.clear();

        // Emplace item
        node.init(std::forward<Args>(args)...);

        return NodeRef(node);
      }
    }

    NodeRef get() noexcept(IF_RELEASE) {
      while (true) {
        if (busy_.test_and_set()) {
          continue;
        }

        // Head is caught up - queue is empty
        if (push_index_ == pop_index_ and size_ == 0) {
          busy_.clear();
          return {};
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto &node = *reinterpret_cast<Node *>(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            raw_data_.data() + element_size_ * pop_index_);

        // Capture node if not busy
        if (node.busy.test_and_set()) {
          busy_.clear();
          continue;
        }

        // Go to next item
        pop_index_ = (pop_index_ + 1) % capacity_;

        assert(size_ > 0);
        --size_;

        busy_.clear();

        return NodeRef(node);
      }
    }

   private:
    const size_t capacity_;
    const size_t element_size_;
    std::vector<std::byte> raw_data_;
    std::atomic_size_t size_ = 0;
    std::atomic_size_t push_index_ = 0;
    std::atomic_size_t pop_index_ = 0;
    mutable std::atomic_flag busy_ = false;
  };

}  // namespace soralog
