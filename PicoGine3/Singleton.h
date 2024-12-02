#ifndef SINGLETON_H
#define SINGLETON_H

#include <mutex>

template <typename T>
class Singleton
{
public:
	static T& Get()
	{
		static T instance{};
		return instance;
	}

	virtual ~Singleton() = default;
	Singleton(const Singleton& other) noexcept = delete;
	Singleton(Singleton&& other) noexcept = delete;
	Singleton& operator=(const Singleton& other) noexcept = delete;
	Singleton& operator=(Singleton&& other) noexcept = delete;

protected:
	Singleton() = default;
};

#endif // SINGLETON_H
