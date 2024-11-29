#ifndef SINGLETON_H
#define SINGLETON_H

#include <mutex>

template <typename T>
class Singleton {
public:
    // Delete copy/move constructor and assignment operator to prevent copies/moves
    Singleton(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton& operator=(Singleton&&) = delete;

    // Access to the Singleton instance
    static T& Get() {
        std::call_once(initFlag, []() {
            instance = new T();
            std::atexit(&Destroy);
            });
        return *instance;
    }

protected:
    // Protected constructor to prevent direct instantiation
    Singleton() = default;
    virtual ~Singleton() = default;

private:
    static void Destroy() {
        delete instance;
        instance = nullptr;
    }

    static T* instance;
    static std::once_flag initFlag;
};

template <typename T>
T* Singleton<T>::instance = nullptr;

template <typename T>
std::once_flag Singleton<T>::initFlag;

#endif // SINGLETON_H
