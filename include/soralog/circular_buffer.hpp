/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_CIRCULARBUFFER
#define SORALOG_CIRCULARBUFFER

#include <vector>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <optional>

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

    class Node {
      std::array<std::byte, sizeof(T)> data_;

     public:
      const T &item = *reinterpret_cast<T *>(data_.data());  // NOLINT
      std::atomic_bool ready{false};

      template <typename... Args>
      explicit Node(const Args &... args) noexcept(IF_RELEASE) {
        new (data_.data()) T(args...);
      }
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
        node_opt->get().ready = ready_after_release;
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

    explicit CircularBuffer(size_t capacity) : data_(capacity){};

    size_t capacity() const noexcept {
      return data_.size();
    }

    size_t size() const noexcept {
      return size_;
    }

    size_t avail() const noexcept {
      return data_.size() - size_;
    }

    template <typename... Args>
    [[nodiscard]] NodeRef put(const Args &... args) noexcept(IF_RELEASE) {
      while (true) {
        auto push_index = push_index_.load();
        auto next_index = (push_index + 1) % data_.size();

        // Tail is caught up - queue is full
        if (pop_index_.load() == next_index) {
          return {};
        }

        auto &node = data_[push_index];

        // Item has not consumed yet
        if (node.ready.load()) {
          continue;
        }

        // Go to next item place
        if (not push_index_.compare_exchange_weak(push_index, next_index,
                                                  std::memory_order_relaxed)) {
          continue;
        }

        size_ = ((push_index_ < pop_index_) ? data_.size() : 0) + push_index_ - pop_index_;

        // Emplace item
        new (&node) Node(args...);
        return NodeRef{node, true};
      }
    }

    NodeRef get() noexcept(IF_RELEASE) {
      while (true) {
        auto pop_index = pop_index_.load();

        // Head is caught up - queue is empty
        if (push_index_.load() == pop_index) {
          return {};
        }

        auto &node = data_[pop_index];

        // Item is already consumed
        if (not node.ready.load()) {
          continue;
        }

        // Go to next item
        if (not pop_index_.compare_exchange_weak(pop_index, (pop_index + 1) % data_.size(),
                                                 std::memory_order_relaxed)) {
          continue;
        }

        size_ = ((push_index_ < pop_index_) ? data_.size() : 0) + push_index_ - pop_index_;

        return NodeRef{node, false};
      }
    }

   private:
    size_t size_ = 0;
    std::vector<Node> data_;
    std::atomic_size_t push_index_ = 0;
    std::atomic_size_t pop_index_ = 0;
  };

}  // namespace soralog

#endif  // SORALOG_CIRCULARBUFFER
