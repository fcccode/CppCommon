/*!
    \file wait_ring.inl
    \brief Multiple producers / multiple consumers wait ring inline implementation
    \author Ivan Shynkarenka
    \date 05.10.2016
    \copyright MIT License
*/

namespace CppCommon {

template<typename T>
inline WaitRing<T>::WaitRing(size_t capacity) : _destroyed(false), _capacity(capacity - 1), _mask(capacity - 1), _head(0), _tail(0), _ring(capacity)
{
    assert((capacity > 1) && "Ring capacity must be greater than one!");
    assert(((capacity & (capacity - 1)) == 0) && "Ring capacity must be a power of two!");
}

template<typename T>
inline WaitRing<T>::~WaitRing()
{
    Locker<CriticalSection> locker(_cs);
    _destroyed = true;
    _cv1.NotifyAll();
    _cv2.NotifyAll();
}

template<typename T>
inline size_t WaitRing<T>::size() const
{
    Locker<CriticalSection> locker(_cs);
    return _head - _tail;
}

template<typename T>
inline bool WaitRing<T>::Enqueue(const T& item)
{
    T temp = item;
    return Enqueue(std::forward<T>(temp));
}

template<typename T>
inline bool WaitRing<T>::Enqueue(T&& item)
{
    Locker<CriticalSection> locker(_cs);

    if (_destroyed)
        return false;

    do
    {
        if (((_head - _tail + 1) & _mask) != 0)
        {
            _ring[_head++ & _mask] = std::move(item);
            _cv1.NotifyOne();
            return true;
        }

        _cv2.Wait(_cs, [this]() { return (_destroyed || (((_head - _tail + 1) & _mask) != 0)); });

    } while (!_destroyed);

    return false;
}

template<typename T>
inline bool WaitRing<T>::Dequeue(T& item)
{
    Locker<CriticalSection> locker(_cs);

    if (_destroyed)
        return false;

    do
    {
        if (((_head - _tail) & _mask) != 0)
        {
            item = std::move(_ring[_tail++ & _mask]);
            _cv2.NotifyOne();
            return true;
        }

        _cv1.Wait(_cs, [this]() { return (_destroyed || (((_head - _tail) & _mask) != 0)); });

    } while (!_destroyed);

    return false;
}

} // namespace CppCommon
