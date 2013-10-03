/*
    Texel - A UCI chess engine.
    Copyright (C) 2013  Peter Österlund, peterosterlund2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * heap.hpp
 *
 *  Created on: Oct 3, 2013
 *      Author: petero
 */

#ifndef HEAP_HPP_
#define HEAP_HPP_

#include <vector>
#include <type_traits>
#include <iostream>

template <typename T> class Heap;

/** A Heap that holds pointers to elements of type T.
 * T must inherit from HeapObject.
 * Destructing a T removes it from its heap.
 */
template <typename T>
class Heap {
public:
    /** Base class for objects that can be inserted in a heap. */
    class HeapObject {
    public:
        HeapObject();
        ~HeapObject();

    private:
        friend class Heap;

        HeapObject(const HeapObject&) = delete;
        HeapObject& operator=(const HeapObject&) = delete;

        Heap<T>* owner;
        int prio;
        int heapIdx;
    };

    /** Constructor. Creates an empty heap. */
    Heap();

    /** Destructor. Removes all elements from heap. */
    ~Heap();

    /** Insert an element in the heap. */
    void insert(T* e, int prio);

    /** Removes an element from the heap. */
    void remove(T* e);

    /** Changes the priority of an element in the heap. */
    void newPrio(T* e, int prio);

    /** Get the element with the highest priority. The element is not removed.
     * Returns null if heap is empty. */
    T* front() const;

    void print(std::ostream& os) const;

private:
    Heap(const Heap& other) = delete;
    Heap& operator=(const Heap& other) = delete;

    /** Swap two elements in the heap vector and update heapIdx. */
    void swapElems(int idx1, int idx2);

    /** Calls upHeap() or downHeap() as needed to restore heap condition. */
    void fixHeap(int idx);

    /** Moves an element up until the heap condition is satisfied.*/
    void upHeap(int idx);

    /** Moves an element down until the heap condition is satisfied. */
    void downHeap(int idx);

    /** Vector of heap elements. */
    std::vector<T*> heap;
};

template <typename T> inline Heap<T>::Heap() {
    static_assert(std::is_base_of<HeapObject,T>::value, "T must inherit HeapObject");
}

template <typename T> inline Heap<T>::~Heap() {
    for (int i = heap.size() - 1; i >= 0; i--)
        remove(heap[i]);
}

template <typename T> inline void Heap<T>::insert(T* e, int prio) {
    e->owner = this;
    e->prio = prio;
    int idx = heap.size();
    e->heapIdx = idx;
    heap.push_back(e);
    upHeap(idx);
}

template <typename T> inline void Heap<T>::remove(T* e) {
    int idx = e->heapIdx;
    int last = heap.size() - 1;
    if (idx < last)
        swapElems(e->heapIdx, last);
    e->owner = nullptr;
    e->heapIdx = -1;
    heap.pop_back();
    if (idx < last)
        fixHeap(idx);
}

template <typename T> inline void Heap<T>::newPrio(T* e, int prio) {
    e->prio = prio;
    fixHeap(e->heapIdx);
}

template <typename T> inline T* Heap<T>::front() const {
    if (heap.empty())
        return nullptr;
    return static_cast<T*>(heap[0]);
}

template <typename T> inline void Heap<T>::swapElems(int idx1, int idx2) {
    std::swap(heap[idx1]->heapIdx, heap[idx2]->heapIdx);
    std::swap(heap[idx1], heap[idx2]);
}

template <typename T> inline void Heap<T>::fixHeap(int idx) {
    int parent = (idx - 1) / 2;
    if ((idx > 0) && (heap[parent]->prio < heap[idx]->prio)) {
        swapElems(idx, parent);
        upHeap(parent);
    } else {
        downHeap(idx);
    }
}

template <typename T> inline void Heap<T>::upHeap(int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (heap[parent]->prio >= heap[idx]->prio)
            break;
        swapElems(idx, parent);
        idx = parent;
    }
}

template <typename T> inline void Heap<T>::downHeap(int idx) {
    int hSize = heap.size();
    while (true) {
        int child = idx * 2 + 1;
        if (child >= hSize)
            break;
        if ((child + 1 < hSize) && (heap[child]->prio < heap[child+1]->prio))
            child++;
        if (heap[idx]->prio >= heap[child]->prio)
            break;
        swapElems(idx, child);
        idx = child;
    }
}

template <typename T> inline Heap<T>::HeapObject::HeapObject()
    : owner(nullptr), prio(0), heapIdx(-1) {
}

template <typename T> inline Heap<T>::HeapObject::~HeapObject() {
    if (owner)
        owner->remove(static_cast<T*>(this));
}

template <typename T> inline void Heap<T>::print(std::ostream& os) const {
    int hSize = heap.size();
    for (int i = 0; i < hSize; i++)
        os << std::setw(2) << i << ' ';
    os << '\n';
    for (int i = 0; i < hSize; i++)
        os << std::setw(2) << heap[i]->prio << ' ';
    os << '\n';
}

#endif /* HEAP_HPP_ */
