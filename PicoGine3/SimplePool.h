//#ifndef SIMPLEPOOL_H
//#define SIMPLEPOOL_H
//
//template<typename ObjType>
//class SimplePool final
//{
//public:
//	explicit SimplePool(std::function<void(ObjType)>&& freeFunction);
//	~SimplePool() = default;
//
//	SimplePool(const SimplePool&) noexcept = delete;
//	SimplePool& operator=(const SimplePool&) noexcept = delete;
//	SimplePool(SimplePool&&) noexcept = delete;
//	SimplePool& operator=(SimplePool&&) noexcept = delete;
//
//	uint16_t Acquire();
//	void Free(uint16_t index);
//
//private:
//	std::vector<ObjType> m_Objects;
//	std::queue<uint16_t> m_FreeSlots;
//	std::function<void(ObjType)> m_FreeFunction;
//};
//
//template <typename ObjType>
//SimplePool<ObjType>::SimplePool(std::function<void(ObjType)>&& freeFunction) :
//	m_FreeFunction{ std::move(freeFunction) }
//{
//}
//
//template <typename ObjType>
//uint16_t SimplePool<ObjType>::Acquire()
//{
//	if (m_FreeSlots.empty())
//	{
//		m_Objects.emplace_back(ObjType{});
//		return static_cast<uint16_t>(m_Objects.size());
//	}
//	else
//	{
//		const uint16_t index{ m_FreeSlots.front() };
//		m_FreeSlots.pop();
//		m_Objects[index] = ObjType{};
//		return index;
//	}
//}
//
//template <typename ObjType>
//void SimplePool<ObjType>::Free(uint16_t index)
//{
//	m_FreeFunction(m_Objects[index]);
//	m_FreeSlots.push(index);
//}
//
//#endif //SIMPLEPOOL_H