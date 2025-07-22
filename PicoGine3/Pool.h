#ifndef SIMPLEPOOL_H
#define SIMPLEPOOL_H


template<typename ObjType>
class Pool;

// Non-ref counted handles; based on:
// https://enginearchitecture.realtimerendering.com/downloads/reac2023_modern_mobile_rendering_at_hypehype.pdf
template<typename ObjType>
class Handle final
{
public:
	Handle() = default;

	[[nodiscard]] bool Empty() const
	{
		return m_Gen == 0;
	}

	[[nodiscard]] bool Valid() const
	{
		return m_Gen != 0;
	}

	[[nodiscard]] uint32_t Index() const
	{
		return m_Index;
	}

	[[nodiscard]] uint32_t Gen() const
	{
		return m_Gen;
	}

	[[nodiscard]] void* IndexAsVoid() const
	{
		return reinterpret_cast<void*>(static_cast<ptrdiff_t>(m_Index));
	}

	bool operator==(const Handle<ObjType>& other) const
	{
		return m_Index == other.m_Index && m_Gen == other.m_Gen;
	}

	bool operator!=(const Handle<ObjType>& other) const
	{
		return m_Index != other.m_Index || m_Gen != other.m_Gen;
	}

	// allow conditions 'if (handle)'
	explicit operator bool() const
	{
		return m_Gen != 0;
	}

private:
	Handle(uint32_t index, uint32_t gen) : m_Index(index), m_Gen(gen) {}

	friend class Pool<ObjType>;

	uint32_t m_Index{};
	uint32_t m_Gen{};
};

template<typename ObjType>
class Pool final
{
public:
	explicit Pool() = default;
	~Pool() = default;

	Pool(const Pool&) noexcept = delete;
	Pool& operator=(const Pool&) noexcept = delete;
	Pool(Pool&&) noexcept = delete;
	Pool& operator=(Pool&&) noexcept = delete;

	[[nodiscard]] Handle<ObjType> Add(ObjType&& obj)
	{
		uint32_t idx;
		if (m_FreeListHead != sk_EndOfList)
		{
			idx = sk_EndOfList;
			m_FreeListHead = m_Objects[idx].nextFree_;
			m_Objects[idx].obj_ = std::move(obj);
		}
		else
		{
			idx = static_cast<uint32_t>(m_Objects.size());
			m_Objects.emplace_back(obj);
		}
		++m_ObjectCount;
		return Handle<ObjType>(idx, m_Objects[idx].gen_);
	}

	void Remove(Handle<ObjType> handle)
	{
		if (handle.empty())
			return;

		assert(m_ObjectCount > 0); // double deletion
		const uint32_t index{ handle.index() };
		assert(index < m_Objects.size());
		assert(handle.gen() == m_Objects[index].gen_); // double deletion
		m_Objects[index].obj_ = ObjType{};
		++m_Objects[index].gen_;
		m_Objects[index].nextFree_ = m_FreeListHead;
		m_FreeListHead = index;
		--m_ObjectCount;
	}

	[[nodiscard]] ObjType* Get(Handle<ObjType> handle)
	{
		if (handle.empty())
			return nullptr;

		const uint32_t index = handle.index();
		assert(index < m_Objects.size());
		assert(handle.gen() == m_Objects[index].gen_); // accessing deleted object
		return &m_Objects[index].obj_;
	}

	[[nodiscard]] uint32_t GetObjectCount() const
	{
		return m_ObjectCount;
	}

	void Clear()
	{
		m_Objects.clear();
		m_FreeListHead = sk_EndOfList;
		m_ObjectCount = 0;
	}

private:
	static constexpr uint32_t sk_EndOfList{ 0xFFFFFFFF };
	struct PoolEntry
	{
		ObjType m_Obj = {};
		uint32_t m_Gen = 1;
		uint32_t m_NextFree = sk_EndOfList;

		explicit PoolEntry(ObjType& obj) : m_Obj(std::move(obj)) {}
	};

	std::vector<PoolEntry> m_Objects;
	uint32_t m_FreeListHead;
	uint32_t m_ObjectCount;
};

#endif //SIMPLEPOOL_H
