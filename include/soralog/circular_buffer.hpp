/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

#ifdef NDEBUG
#define IF_RELEASE true
#else
#define IF_RELEASE false
#endif

/**
 * @brief Implements a circular buffer for log events without locks.
 *
 * This buffer is used for efficient, thread-safe logging. It provides
 * FIFO storage and retrieval of events using atomic operations only; see
 * CircularBuffer for its progress guarantees.
 */

namespace soralog {

  /**
   * @class CircularBuffer
   * @brief A bounded MPMC queue with fixed capacity and in-place elements.
   *
   * Implements the bounded queue of Dmitry Vyukov. Every slot carries a
   * sequence number telling whose turn it is; with `pos` being a position
   * in the infinite stream of elements and the slot being `pos % capacity`:
   *  - `2 * pos`                    — the slot is free for the producer of
   *                                   position `pos`;
   *  - `2 * pos + 1`                — the slot holds the published element of
   *                                   position `pos`, ready for a consumer;
   *  - `2 * (pos + capacity)`       — a consumer has released the slot, it is
   *                                   free for the producer of the next lap.
   * Doubling keeps "published" (odd) apart from "free" (even) even for
   * capacity 1, where the original scheme is ambiguous.
   *
   * A position is claimed by CAS on the shared counter of its side, so the
   * only contended operation is one CAS per element. The element is built
   * right in the slot and published when the returned NodeRef is destroyed;
   * a consumer reads it in place and releases the slot the same way.
   *
   * Not lock-free in the strict sense: a producer suspended between claiming
   * a position and publishing it holds back consumption of the elements after
   * it. Nothing is lost - get() returns nothing until the element is
   * published (see has_ready()).
   *
   * Capacity must be positive; it is checked once, in the constructor.
   *
   * Elements are never destroyed explicitly, so T must be trivially
   * destructible.
   *
   * @tparam T Type of elements stored in the buffer.
   */
  template <typename T>
  class CircularBuffer final {
    static_assert(std::is_trivially_destructible_v<T>,
                  "Elements of CircularBuffer are never destroyed explicitly");

   public:
    using element_type = T;

    /**
     * @struct Node
     * @brief Represents a buffer node storing a single element.
     */
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    struct Node final {
      // item_ is raw storage: the element is constructed in place later
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
      explicit Node(size_t seq) noexcept : seq(seq) {}

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
        return *std::launder(reinterpret_cast<const T *>(item_));
      }

      // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
      std::atomic_size_t seq;  ///< Turn of the slot, see class description.

     private:
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
      alignas(std::alignment_of_v<T>) char item_[sizeof(T)];
    };

    /**
     * @class NodeRef
     * @brief Ownership of a slot: publishes or releases it when destroyed.
     */
    class NodeRef {
      Node *node_ = nullptr;
      size_t seq_ = 0;  ///< Sequence number to store on destruction

     public:
      NodeRef() noexcept = default;
      NodeRef(NodeRef &&) noexcept = delete;
      NodeRef(const NodeRef &) = delete;
      NodeRef &operator=(NodeRef &&) noexcept = delete;
      NodeRef &operator=(const NodeRef &) = delete;

      NodeRef(Node &node, size_t seq) noexcept : node_(&node), seq_(seq) {}

      /**
       * @brief Publishes (producer) or releases (consumer) the slot.
       */
      ~NodeRef() noexcept(IF_RELEASE) {
        if (node_) {
          node_->seq.store(seq_, std::memory_order_release);
        }
      }

      /**
       * @brief Accesses the stored element.
       * @return Reference to the element.
       */
      const T &operator*() const noexcept(IF_RELEASE) {
        assert(node_);
        return node_->item();
      }

      /**
       * @brief Accesses the stored element.
       * @return Pointer to the element.
       */
      const T *operator->() const noexcept(IF_RELEASE) {
        assert(node_);
        return &(node_->item());
      }

      /**
       * @brief Checks if the reference holds a valid node.
       * @return True if valid, false otherwise.
       */
      explicit operator bool() const noexcept {
        return node_ != nullptr;
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
     * @param padding Extra space after each element (e.g. for event text).
     * @throws std::invalid_argument if capacity is zero.
     */
    CircularBuffer(size_t capacity, size_t padding)
        : capacity_(checkCapacity(capacity)),
          element_size_(roundUp(sizeof(Node) + padding, kSlotAlignment)),
          raw_data_(allocate(capacity_ * element_size_)) {
      for (size_t index = 0; index < capacity_; ++index) {
        new (slot(index)) Node(2 * index);
      }
    };

    /**
     * @brief Constructs a circular buffer with no padding.
     * @param capacity Maximum number of elements.
     * @throws std::invalid_argument if capacity is zero.
     */
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    explicit CircularBuffer(size_t capacity) : CircularBuffer(capacity, 0) {};

    /**
     * @brief Gets the buffer capacity.
     * @return Maximum number of elements.
     */
    size_t capacity() const noexcept {
      return capacity_;
    }

    /**
     * @brief Gets the number of elements currently stored.
     *
     * Counts claimed positions, including elements being written by producers
     * right now. Exact when the buffer is quiescent, a snapshot otherwise.
     *
     * @return Number of elements in the buffer.
     */
    size_t size() const noexcept {
      // Head first: the tail read after it is never behind it
      const auto head = dequeue_pos_.load(std::memory_order_acquire);
      const auto tail = enqueue_pos_.load(std::memory_order_acquire);
      return std::min(tail - head, capacity_);
    }

    /**
     * @brief Gets the available space in the buffer.
     * @return Number of positions not claimed. A slot taken by get() counts
     * as free while its NodeRef is still alive, so put() may fail anyway.
     */
    size_t avail() const noexcept {
      return capacity_ - size();
    }

    /**
     * @brief Checks whether the oldest element is published and can be got.
     *
     * Unlike size() it ignores elements being written right now. Intended for
     * a consumer that is about to stop: after the last get() returned nothing
     * it tells whether something was published in the meantime.
     *
     * @return True if get() would return an element.
     */
    bool has_ready() const noexcept {
      const auto pos = dequeue_pos_.load(std::memory_order_acquire);
      return node(pos).seq.load(std::memory_order_acquire) == 2 * pos + 1;
    }

    /**
     * @brief Adds a new element to the buffer.
     *
     * The element is published when the returned reference is destroyed.
     *
     * @tparam Args Argument types for constructing the element.
     * @param args Arguments forwarded to the element's constructor.
     * @return A reference to the created node, or an empty reference if full.
     */
    template <typename... Args>
    [[nodiscard]] NodeRef put(Args &&...args) noexcept(IF_RELEASE) {
      auto pos = enqueue_pos_.load(std::memory_order_relaxed);
      while (true) {
        auto &slot = node(pos);
        const auto seq = slot.seq.load(std::memory_order_acquire);
        const auto diff = static_cast<std::ptrdiff_t>(seq)
                        - static_cast<std::ptrdiff_t>(2 * pos);
        if (diff == 0) {
          // Slot is free for this position: claim the position
          if (enqueue_pos_.compare_exchange_weak(
                  pos, pos + 1, std::memory_order_relaxed)) {
            slot.init(std::forward<Args>(args)...);
            return NodeRef(slot, 2 * pos + 1);
          }
          // Failed CAS has reloaded pos
        } else if (diff < 0) {
          // Slot still belongs to the previous lap: buffer is full
          return {};
        } else {
          // Another producer has claimed this position
          pos = enqueue_pos_.load(std::memory_order_relaxed);
        }
      }
    }

    /**
     * @brief Retrieves and removes the oldest element from the buffer.
     *
     * The slot is released when the returned reference is destroyed.
     *
     * @return A reference to the retrieved node, or empty if there is no
     * published element.
     */
    NodeRef get() noexcept(IF_RELEASE) {
      auto pos = dequeue_pos_.load(std::memory_order_relaxed);
      while (true) {
        auto &slot = node(pos);
        const auto seq = slot.seq.load(std::memory_order_acquire);
        const auto diff = static_cast<std::ptrdiff_t>(seq)
                        - static_cast<std::ptrdiff_t>(2 * pos + 1);
        if (diff == 0) {
          // Element is published: claim the position
          if (dequeue_pos_.compare_exchange_weak(
                  pos, pos + 1, std::memory_order_relaxed)) {
            return NodeRef(slot, 2 * (pos + capacity_));
          }
          // Failed CAS has reloaded pos
        } else if (diff < 0) {
          // Buffer is empty, or the element is not published yet
          return {};
        } else {
          // Another consumer has claimed this position
          pos = dequeue_pos_.load(std::memory_order_relaxed);
        }
      }
    }

   private:
    /// Slots and counters are kept apart by this distance to avoid false
    /// sharing. GCC warns that the value depends on -mtune (and so does the
    /// layout of this class); the warning is silenced deliberately.
#ifdef __cpp_lib_hardware_interference_size
#if defined(__GNUC__) and not defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winterference-size"
#endif
    static constexpr size_t kCacheLine =
        std::hardware_destructive_interference_size;
#if defined(__GNUC__) and not defined(__clang__)
#pragma GCC diagnostic pop
#endif
#else
    // Covers adjacent-line prefetch on x86 and the line of Apple silicon
    static constexpr size_t kCacheLine = 128;
#endif
    static constexpr size_t kSlotAlignment =
        std::max(kCacheLine, alignof(Node));

    struct Deleter {
      void operator()(std::byte *ptr) const noexcept {
        ::operator delete(ptr, std::align_val_t{kSlotAlignment});
      }
    };
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
    using Storage = std::unique_ptr<std::byte[], Deleter>;

    static size_t checkCapacity(size_t capacity) {
      if (capacity == 0) {
        throw std::invalid_argument(
            "Capacity of CircularBuffer must be positive");
      }
      return capacity;
    }

    static constexpr size_t roundUp(size_t value, size_t alignment) {
      return (value + alignment - 1) / alignment * alignment;
    }

    static Storage allocate(size_t size) {
      return Storage(static_cast<std::byte *>(
          ::operator new(size, std::align_val_t{kSlotAlignment})));
    }

    std::byte *slot(size_t index) const noexcept {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      return raw_data_.get() + element_size_ * index;
    }

    Node &node(size_t pos) const noexcept {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      return *std::launder(reinterpret_cast<Node *>(slot(pos % capacity_)));
    }

    const size_t capacity_;      ///< Maximum number of elements.
    const size_t element_size_;  ///< Size of each slot.
    Storage raw_data_;           ///< Slots storage.

    /// Next position to produce into
    alignas(kCacheLine) std::atomic_size_t enqueue_pos_ = 0;
    /// Next position to consume from
    alignas(kCacheLine) std::atomic_size_t dequeue_pos_ = 0;
  };

}  // namespace soralog
