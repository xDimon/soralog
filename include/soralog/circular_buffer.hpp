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

/**
 * @brief Implements a lock-free circular buffer.
 *
 * This buffer is used for efficient, thread-safe logging. It provides
 * FIFO storage and retrieval of events using atomic operations.
 */

namespace soralog {

  /**
   * @class CircularBuffer
   * @brief A lock-free circular buffer with fixed capacity.
   *
   * Used in logging systems to temporarily store events before writing.
   * Uses atomic operations to ensure thread safety.
   *
   * @tparam T Type of elements stored in the buffer.
   */
  template <typename T>
  class CircularBuffer final {
   public:
    using element_type = T;

    /**
     * @struct Node
     * @brief Represents a buffer node storing a single element.
     */
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    struct Node final {
      /**
       * @brief Initializes the node with an element.
       * @tparam Args Argument types for constructing the element.
       * @param args Arguments forwarded to the element's constructor.
       */
      template <typename... Args>
      void init(Args &&...args) {
        new (item_) T(std::forward<Args>(args)...);
      }

      /**
       * @brief Retrieves the stored element.
       * @return Reference to the element.
       */
      inline const T &item() const {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<const T *>(item_);
      }

      // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
      std::atomic_flag busy{false};  ///< Flag indicating if the node is in use.

     private:
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
      alignas(std::alignment_of_v<T>) char item_[sizeof(T)];
    };

    /**
     * @class NodeRef
     * @brief A smart reference to a buffer node ensuring safe access.
     */
    class NodeRef {
      Node *node = nullptr;

     public:
      NodeRef() noexcept = default;
      NodeRef(NodeRef &&) noexcept = delete;
      NodeRef(const NodeRef &) = delete;
      NodeRef &operator=(NodeRef &&) noexcept = delete;
      NodeRef &operator=(const NodeRef &) = delete;

      explicit NodeRef(Node &node) noexcept : node(&node) {}

      /**
       * @brief Releases the node when reference goes out of scope.
       */
      ~NodeRef() noexcept(IF_RELEASE) {
        if (node) {
          node->busy.clear();
        }
      }

      /**
       * @brief Accesses the stored element.
       * @return Reference to the element.
       */
      const T &operator*() const noexcept(IF_RELEASE) {
        assert(node);
        return node->item();
      }

      /**
       * @brief Accesses the stored element.
       * @return Pointer to the element.
       */
      const T *operator->() const noexcept(IF_RELEASE) {
        assert(node);
        return &(node->item());
      }

      /**
       * @brief Checks if the reference holds a valid node.
       * @return True if valid, false otherwise.
       */
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

    /**
     * @brief Constructs a circular buffer.
     * @param capacity Maximum number of elements.
     * @param padding Extra padding for each node.
     */
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

    /**
     * @brief Constructs a circular buffer with no padding.
     * @param capacity Maximum number of elements.
     */
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    explicit CircularBuffer(size_t capacity) : CircularBuffer(capacity, 0) {};

    /**
     * @brief Gets the buffer capacity.
     * @return Maximum number of elements.
     */
    size_t capacity() const noexcept {
      while (busy_.test_and_set()) {
        continue;
      }
      auto ret = capacity_;
      busy_.clear();
      return ret;
    }

    /**
     * @brief Gets the number of elements currently stored.
     * @return Number of elements in the buffer.
     */
    size_t size() const noexcept {
      while (busy_.test_and_set()) {
        continue;
      }
      auto ret = size_.load();
      busy_.clear();
      return ret;
    }

    /**
     * @brief Gets the available space in the buffer.
     * @return Number of free slots.
     */
    size_t avail() const noexcept {
      while (busy_.test_and_set()) {
        continue;
      }
      auto ret = capacity_ - size_;
      busy_.clear();
      return ret;
    }

    /**
     * @brief Adds a new element to the buffer.
     * @tparam Args Argument types for constructing the element.
     * @param args Arguments forwarded to the element's constructor.
     * @return A reference to the created node, or an empty reference if full.
     */
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

    /**
     * @brief Retrieves and removes the oldest element from the buffer.
     * @return A reference to the retrieved node, or empty if buffer is empty.
     */
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
    const size_t capacity_;                  ///< Maximum number of elements.
    const size_t element_size_;              ///< Size of each element.
    std::vector<std::byte> raw_data_;        ///< Raw buffer storage.
    std::atomic_size_t size_ = 0;            ///< Current number of elements.
    std::atomic_size_t push_index_ = 0;      ///< Index for adding elements.
    std::atomic_size_t pop_index_ = 0;       ///< Index for removing elements.
    mutable std::atomic_flag busy_ = false;  ///< Lock flag.
  };

}  // namespace soralog
