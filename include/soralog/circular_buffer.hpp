/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_CIRCULARBUFFER
#define SORALOG_CIRCULARBUFFER

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
    class Node final {
     public:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      const T &item = *reinterpret_cast<T *>(data_.data());
#pragma GCC diagnostic pop
      std::atomic_bool ready;

      template <typename... Args>
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
      explicit Node(const Args &...args) noexcept(IF_RELEASE) {
        new (data_.data()) T(args...);
        ready.store(false, std::memory_order_release);
      }

     private:
      std::array<std::byte, sizeof(T)> data_;
    };

    class NodeRef {
      using base_t = std::optional<std::reference_wrapper<Node>>;

      base_t node_opt{};
      bool ready_after_release{};

     public:
      NodeRef() noexcept = default;
      NodeRef(NodeRef &&) noexcept = delete;
      NodeRef(const NodeRef &) = delete;
      NodeRef &operator=(NodeRef &&) noexcept = delete;
      NodeRef &operator=(NodeRef const &) = delete;

      NodeRef(Node &node, bool ready_after_release) noexcept
          : node_opt(std::ref(node)),
            ready_after_release(ready_after_release) {}

      ~NodeRef() noexcept(IF_RELEASE) {
        if (node_opt.has_value()) {
          release();
        }
      }

      void release() noexcept(IF_RELEASE) {
        assert(node_opt.has_value());
        node_opt->get().ready.store(ready_after_release,
                                    std::memory_order_release);
        node_opt.reset();
      }

      const T &operator*() const noexcept(IF_RELEASE) {
        assert(node_opt.has_value());
        return node_opt->get().item;
      }

      const T *operator->() const noexcept(IF_RELEASE) {
        assert(node_opt.has_value());
        return &node_opt->get().item;
      }

      explicit operator bool() const noexcept {
        return node_opt.has_value();
      }
    };

    CircularBuffer() = delete;
    CircularBuffer(CircularBuffer &&) noexcept = delete;
    CircularBuffer(const CircularBuffer &) = delete;
    ~CircularBuffer() = default;
    CircularBuffer &operator=(CircularBuffer &&) noexcept = delete;
    CircularBuffer &operator=(CircularBuffer const &) = delete;

    explicit CircularBuffer(size_t capacity)
        : capacity_(capacity),
          element_size_(sizeof(Node)),
          raw_data_(capacity_ * element_size_){};

    CircularBuffer(size_t capacity, size_t padding)
        : capacity_(capacity),
          element_size_([&] {
            const auto alignment = std::alignment_of_v<Node>;
            if (padding % alignment) {
              padding += alignment - padding % alignment;
            }
            return sizeof(Node) + padding;
          }()),
          raw_data_(capacity_ * element_size_){};

    size_t capacity() const noexcept {
      return capacity_;
    }

    size_t size() const noexcept {
      return size_;
    }

    size_t avail() const noexcept {
      return capacity_ - size_;
    }

    template <typename... Args>
    [[nodiscard]] NodeRef put(const Args &...args) noexcept(IF_RELEASE) {
      while (true) {
        auto push_index = push_index_.load(std::memory_order_acquire);
        auto next_index = (push_index + 1) % capacity_;

        // Tail is caught up - queue is full
        auto pop_index = pop_index_.load(std::memory_order_acquire);
        if (pop_index == next_index) {
          return {};
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto &node = *reinterpret_cast<Node *>(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            raw_data_.data() + element_size_ * push_index);

        // Item has not consumed yet
        if (node.ready.load(std::memory_order_acquire)) {
          continue;
        }

        // Go to next item place
        if (not push_index_.compare_exchange_weak(push_index, next_index,
                                                  std::memory_order_release)) {
          continue;
        }

        size_ = ((next_index < pop_index) ? capacity_ : 0)
            + (next_index - pop_index);

        // Emplace item
        new (&node) Node(args...);
        return NodeRef{node, true};
      }
    }

    NodeRef get() noexcept(IF_RELEASE) {
      while (true) {
        auto pop_index = pop_index_.load(std::memory_order_acquire);

        // Head is caught up - queue is empty
        auto push_index = push_index_.load(std::memory_order_acquire);
        if (push_index == pop_index) {
          return {};
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto &node = *reinterpret_cast<Node *>(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            raw_data_.data() + element_size_ * pop_index);

        // Item is already consumed
        if (not node.ready.load(std::memory_order_acquire)) {
          continue;
        }

        // Go to next item
        auto next_index = (pop_index + 1) % capacity_;
        if (not pop_index_.compare_exchange_weak(pop_index, next_index,
                                                 std::memory_order_release)) {
          continue;
        }

        size_ = ((push_index < next_index) ? capacity_ : 0)
            + (push_index - next_index);

        return NodeRef{node, false};
      }
    }

   private:
    const size_t capacity_;
    const size_t element_size_;
    std::vector<std::byte> raw_data_;
    std::atomic_size_t size_ = 0;
    std::atomic_size_t push_index_ = 0;
    std::atomic_size_t pop_index_ = 0;
  };

}  // namespace soralog

#endif  // SORALOG_CIRCULARBUFFER
