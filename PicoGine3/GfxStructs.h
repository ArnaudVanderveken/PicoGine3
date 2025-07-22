#ifndef GFXSTRUCTS_H
#define GFXSTRUCTS_H

class GraphicsAPI;
struct GfxBuffer;
struct GfxImage;

#pragma region Shared

static constexpr uint32_t g_MaxColorAttachment{ 8 };

enum IndexFormat : uint8_t
{
	IndexFormat_U8,
	IndexFormat_U16,
	IndexFormat_U32
};

enum LoadOp : uint8_t
{
	LoadOp_Invalid = 0,
	LoadOp_DontCare,
	LoadOp_Load,
	LoadOp_Clear,
	LoadOp_None,
};

enum StoreOp : uint8_t
{
	StoreOp_DontCare = 0,
	StoreOp_Store,
	StoreOp_MsaaResolve,
	StoreOp_None,
};

enum CompareOp : uint8_t
{
	CompareOp_Never = 0,
	CompareOp_Less,
	CompareOp_Equal,
	CompareOp_LessEqual,
	CompareOp_Greater,
	CompareOp_NotEqual,
	CompareOp_GreaterEqual,
	CompareOp_AlwaysPass
};

enum StorageType
{
	StorageType_Device,
	StorageType_HostVisible,
	StorageType_Memoryless
};

enum BufferUsageBits : uint8_t
{
	BufferUsageBits_Index = 1 << 0,
	BufferUsageBits_Vertex = 1 << 1,
	BufferUsageBits_Uniform = 1 << 2,
	BufferUsageBits_Storage = 1 << 3,
	BufferUsageBits_Indirect = 1 << 4,
	// ray tracing
	BufferUsageBits_ShaderBindingTable = 1 << 5,
	BufferUsageBits_AccelStructBuildInputReadOnly = 1 << 6,
	BufferUsageBits_AccelStructStorage = 1 << 7
};

enum CullMode : uint8_t
{
	CullMode_None,
	CullMode_Front,
	CullMode_Back
};

enum WindingMode : uint8_t
{
	WindingMode_CCW,
	WindingMode_CW
};

enum TextureType : uint8_t
{
	TextureType_2D,
	TextureType_3D,
	TextureType_Cube,
};

enum TextureUsageBits : uint8_t
{
	TextureUsageBits_Sampled = 1 << 0,
	TextureUsageBits_Storage = 1 << 1,
	TextureUsageBits_Attachment = 1 << 2,
};

enum Format : uint8_t
{
	Invalid = 0,

	R8_UNorm,
	R16_UNorm,
	R16_UInt,
	R32_UInt,
	R16_SFloat,
	R32_SFloat,

	R8G8_UNorm,
	R16G16_UNorm,
	R16G16_UInt,
	R32G32_UInt,
	R16G16_SFloat,
	R32G32_SFloat,

	R8G8B8A8_UNorm,
	R8G8B8A8_SRGB,
	R32G32B32A32_UInt,
	R16G16B16A16_SFloat,
	R32G32B32A32_SFloat,

	B8G8R8A8_UNorm,
	B8G8R8A8_SRGB,

	ETC2_R8G8B8_UNorm,
	ETC2_R8G8B8_SRGB,
	BC7_UNorm,

	D16_UNorm,
	D24_UNorm_S8_UInt,
	D32_SFloat,
	D32_SFloat_S8_UInt
};

enum Swizzle : uint8_t
{
	Swizzle_Default = 0,
	Swizzle_0,
	Swizzle_1,
	Swizzle_R,
	Swizzle_G,
	Swizzle_B,
	Swizzle_A,
};

union ClearColorValue
{
	float m_Float32[4];
	int32_t m_Int32[4];
	uint32_t m_Uint32[4];
};

struct RenderPass final
{
	struct AttachmentDesc final
	{
		LoadOp m_LoadOp{ LoadOp_Invalid };
		StoreOp m_StoreOp{ StoreOp_Store };
		uint8_t m_Layer{ 0 };
		uint8_t m_Level{ 0 };
		float m_ClearColor[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
		float m_ClearDepth{ 1.0f };
		uint32_t m_ClearStencil{ 0 };
	};

	AttachmentDesc m_Color[g_MaxColorAttachment]{};
	AttachmentDesc m_Depth{ .m_LoadOp = LoadOp_DontCare, .m_StoreOp = StoreOp_DontCare };
	AttachmentDesc m_Stencil{ .m_LoadOp = LoadOp_Invalid, .m_StoreOp = StoreOp_DontCare };

	[[nodiscard]] uint32_t GetNumColorAttachments() const
	{
		uint32_t n{ 0 };

		while (n < g_MaxColorAttachment && m_Color[n].m_LoadOp != LoadOp_Invalid)
			++n;

		return n;
	}
};

struct Framebuffer final
{
	struct AttachmentDesc
	{
		GfxImage* m_Texture;
		GfxImage* m_ResolveTexture;
	};

	AttachmentDesc m_Color[g_MaxColorAttachment]{};
	AttachmentDesc m_DepthStencil;

	const char* m_DebugName{};

	[[nodiscard]] uint32_t GetNumColorAttachments() const
	{
		uint32_t n{ 0 };

		while (n < g_MaxColorAttachment && m_Color[n].m_Texture)
			++n;

		return n;
	}
};

struct Dependencies
{
	static constexpr uint32_t sk_MaxSubmitDependencies{ 4 };
	GfxImage* m_Textures[sk_MaxSubmitDependencies]{};
	GfxBuffer* m_Buffers[sk_MaxSubmitDependencies]{};
};

struct Viewport
{
	float m_X{ 0.0f };
	float m_Y{ 0.0f };
	float m_Width{ 1.0f };
	float m_Height{ 1.0f };
	float m_MinDepth{ 0.0f };
	float m_MaxDepth{ 1.0f };
};

struct ScissorRect
{
	uint32_t m_X{ 0 };
	uint32_t m_Y{ 0 };
	uint32_t m_Width{ 0 };
	uint32_t m_Height{ 0 };
};

struct DepthState
{
	CompareOp m_CompareOp{ CompareOp_AlwaysPass };
	bool m_IsDepthWriteEnabled{ false };
};

struct Dimensions
{
	uint32_t m_Width{ 1 };
	uint32_t m_Height{ 1 };
	uint32_t m_Depth{ 1 };

	inline Dimensions Divide1D(uint32_t v) const
	{
		return { .m_Width = m_Width / v, .m_Height = m_Height, .m_Depth = m_Depth };
	}

	inline Dimensions Divide2D(uint32_t v) const
	{
		return { .m_Width = m_Width / v, .m_Height = m_Height / v, .m_Depth = m_Depth };
	}

	inline Dimensions Divide3D(uint32_t v) const
	{
		return { .m_Width = m_Width / v, .m_Height = m_Height / v, .m_Depth = m_Depth / v };
	}

	inline bool operator==(const Dimensions& other) const
	{
		return m_Width == other.m_Width && m_Height == other.m_Height && m_Depth == other.m_Depth;
	}
};

struct Offset3D
{
	int32_t m_X{ 0 };
	int32_t m_Y{ 0 };
	int32_t m_Z{ 0 };
};

struct TextureLayers
{
	uint32_t m_MipLevel{ 0 };
	uint32_t m_Layer{ 0 };
	uint32_t m_NumLayers{ 1 };
};

struct TextureFormatProperties
{
	Format m_Format;
	uint8_t m_BytesPerBlock = 1;
	uint8_t m_BlockWidth = 1;
	uint8_t m_BlockHeight = 1;
	uint8_t m_MinBlocksX = 1;
	uint8_t m_MinBlocksY = 1;
	bool m_Depth = false;
	bool m_Stencil = false;
	bool m_Compressed = false;
};

struct ComponentMapping
{
	Swizzle m_R = Swizzle_Default;
	Swizzle m_G = Swizzle_Default;
	Swizzle m_B = Swizzle_Default;
	Swizzle m_A = Swizzle_Default;

	[[nodiscard]] bool Identity() const
	{
		return m_R == Swizzle_Default && m_G == Swizzle_Default && m_B == Swizzle_Default && m_A == Swizzle_Default;
	}
};

#define PROPS(fmt, bpb, ...) TextureFormatProperties { .m_Format = ##fmt, .m_BytesPerBlock = bpb, ##__VA_ARGS__ }

static constexpr TextureFormatProperties sk_textureFormatProperties[]{
	PROPS(Invalid, 1),
	PROPS(R8_UNorm, 1),
	PROPS(R16_UNorm, 2),
	PROPS(R16_UInt, 2),
	PROPS(R32_UInt, 4),
	PROPS(R16_SFloat, 2),
	PROPS(R32_SFloat, 4),
	PROPS(R8G8_UNorm, 2),
	PROPS(R16G16_UNorm, 4),
	PROPS(R16G16_UInt, 4),
	PROPS(R32G32_UInt, 8),
	PROPS(R16G16_SFloat, 4),
	PROPS(R32G32_SFloat, 8),
	PROPS(R8G8B8A8_UNorm, 4),
	PROPS(R8G8B8A8_SRGB, 4),
	PROPS(R32G32B32A32_UInt, 16),
	PROPS(R16G16B16A16_SFloat, 8),
	PROPS(R32G32B32A32_SFloat, 16),
	PROPS(B8G8R8A8_UNorm, 4),
	PROPS(B8G8R8A8_SRGB, 4),
	PROPS(ETC2_R8G8B8_UNorm, 8, .m_BlockWidth = 4, .m_BlockHeight = 4, .m_Compressed = true),
	PROPS(ETC2_R8G8B8_SRGB, 8, .m_BlockWidth = 4, .m_BlockHeight = 4, .m_Compressed = true),
	PROPS(BC7_UNorm, 16, .m_BlockWidth = 4, .m_BlockHeight = 4, .m_Compressed = true),
	PROPS(D16_UNorm, 2, .m_Depth = true),
	PROPS(D32_SFloat, 4, .m_Depth = true),
	PROPS(D32_SFloat_S8_UInt, 5, .m_Depth = true, .m_Stencil = true),
	PROPS(D24_UNorm_S8_UInt, 4, .m_Depth = true, .m_Stencil = true)
};

#undef PROPS

inline VkAttachmentLoadOp LoadOpToVkAttachmentLoadOp(LoadOp a)
{
	switch (a)
	{
		case LoadOp_Invalid:
			assert(false);
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		case LoadOp_DontCare:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		case LoadOp_Load:
			return VK_ATTACHMENT_LOAD_OP_LOAD;
		case LoadOp_Clear:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case LoadOp_None:
			return VK_ATTACHMENT_LOAD_OP_NONE_EXT;
	}
	assert(false);
	return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

inline VkAttachmentStoreOp StoreOpToVkAttachmentStoreOp(StoreOp a)
{
	switch (a)
	{
		case StoreOp_DontCare:
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		case StoreOp_Store:
			return VK_ATTACHMENT_STORE_OP_STORE;
		case StoreOp_MsaaResolve:
			// for MSAA resolve, we have to store data into a special "resolve" attachment
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		case StoreOp_None:
			return VK_ATTACHMENT_STORE_OP_NONE;
	}
	assert(false);
	return VK_ATTACHMENT_STORE_OP_DONT_CARE;
}

inline VkMemoryPropertyFlags StorageTypeToVkMemoryPropertyFlags(StorageType storage)
{
	VkMemoryPropertyFlags memFlags{ 0 };

	switch (storage)
	{
	case StorageType_Device:
		memFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		break;
	case StorageType_HostVisible:
		memFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		break;
	case StorageType_Memoryless:
		memFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
		break;
	}
	return memFlags;
}

inline VkFormat FormatToVkFormat(Format format)
{
	switch (format)
	{
	case Invalid:
		return VK_FORMAT_UNDEFINED;

	case R8_UNorm:
		return VK_FORMAT_R8_UNORM;

	case R16_UNorm:
		return VK_FORMAT_R16_UNORM;

	case R16_UInt:
		return VK_FORMAT_R16_UINT;

	case R32_UInt:
		return VK_FORMAT_R32_UINT;

	case R16_SFloat:
		return VK_FORMAT_R16_SFLOAT;

	case R32_SFloat:
		return VK_FORMAT_R32_SFLOAT;

	case R8G8_UNorm:
		return VK_FORMAT_R8G8_UNORM;

	case R16G16_UNorm:
		return VK_FORMAT_R16G16_UNORM;

	case R16G16_UInt:
		return VK_FORMAT_R16G16_UINT;

	case R32G32_UInt:
		return VK_FORMAT_R32G32_UINT;

	case R16G16_SFloat:
		return VK_FORMAT_R16G16_SFLOAT;

	case R32G32_SFloat:
		return VK_FORMAT_R32G32_SFLOAT;

	case R8G8B8A8_UNorm:
		return VK_FORMAT_R8G8B8A8_UNORM;

	case R8G8B8A8_SRGB:
		return VK_FORMAT_R8G8B8A8_SRGB;

	case R32G32B32A32_UInt:
		return VK_FORMAT_R32G32B32A32_UINT;

	case R16G16B16A16_SFloat:
		return VK_FORMAT_R16G16B16A16_SFLOAT;

	case R32G32B32A32_SFloat:
		return VK_FORMAT_R32G32B32A32_SFLOAT;

	case B8G8R8A8_UNorm:
		return VK_FORMAT_B8G8R8A8_UNORM;

	case B8G8R8A8_SRGB:
		return VK_FORMAT_B8G8R8A8_SRGB;

	case ETC2_R8G8B8_UNorm:
		return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;

	case ETC2_R8G8B8_SRGB:
		return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;

	case BC7_UNorm:
		return VK_FORMAT_BC7_UNORM_BLOCK;

	case D16_UNorm:
		return VK_FORMAT_D16_UNORM;

	case D24_UNorm_S8_UInt:
		return VK_FORMAT_D24_UNORM_S8_UINT;

	case D32_SFloat:
		return VK_FORMAT_D32_SFLOAT;

	case D32_SFloat_S8_UInt:
		return VK_FORMAT_D32_SFLOAT_S8_UINT;
	}

	Logger::Get().LogError(std::format(L"Unsupported format : {}", static_cast<int>(format)));
	return VK_FORMAT_UNDEFINED;
}

#pragma endregion

#pragma region GfxBuffer

struct BufferDesc final
{
	uint8_t m_Usage{ 0 };
	StorageType m_Storage{ StorageType_HostVisible };
	size_t m_Size{ 0 };
	const void* m_Data{ nullptr };
	const char* m_DebugName{ nullptr };
};

struct GfxBuffer final
{
	GfxBuffer() = default;
	~GfxBuffer() = default;

	GfxBuffer(const GfxBuffer& other) noexcept = delete;
	GfxBuffer& operator=(const GfxBuffer& other) noexcept = delete;
	GfxBuffer(GfxBuffer&& other) noexcept = default;
	GfxBuffer& operator=(GfxBuffer&& other) noexcept = default;

	[[nodiscard]] inline uint8_t* GetMappedPtr() const;
	[[nodiscard]] inline bool IsMapped() const;

	void WriteBufferData(size_t offset, size_t size, const void* data) const;
	void GetBufferData(size_t offset, size_t size, void* data) const;
	void FlushMappedMemory(VkDeviceSize offset, VkDeviceSize size) const;
	void InvalidateMappedMemory(VkDeviceSize offset, VkDeviceSize size) const;

	VkBuffer m_VkBuffer{ VK_NULL_HANDLE };
	VkDeviceMemory m_VkMemory{ VK_NULL_HANDLE };
	//VmaAllocation m_VmaAllocation{ VK_NULL_HANDLE };
	VkDeviceAddress m_VkDeviceAddress{};
	VkDeviceSize m_BufferSize{};
	VkBufferUsageFlags m_VkUsageFlags{};
	VkMemoryPropertyFlags m_VkMemFlags{};
	void* m_pMappedPtr{ nullptr };
	bool m_IsCoherentMemory{ false };
	GraphicsAPI* m_pGraphicsAPI;
};

#pragma endregion

#pragma region GfxImage

struct StageAccess
{
	VkPipelineStageFlags2 m_Stage;
	VkAccessFlags2 m_Access;
};

struct TextureDesc
{
	TextureType m_Type = TextureType_2D;
	Format m_Format = Invalid;

	Dimensions m_Dimensions = { 1, 1, 1 };
	uint32_t m_NumLayers = 1;
	uint32_t m_NumSamples = 1;
	uint8_t m_Usage = TextureUsageBits_Sampled;
	uint32_t m_NumMipLevels = 1;
	StorageType m_Storage = StorageType_Device;
	ComponentMapping m_Swizzle = {};
	const void* m_Data = nullptr;
	uint32_t m_DataNumMipLevels = 1;
	bool m_GenerateMipmaps = false;
	const char* m_DebugName = "";
};


struct GfxImage final
{
	GfxImage() = default;
	~GfxImage() = default;

	GfxImage(const GfxImage&) noexcept = delete;
	GfxImage& operator=(const GfxImage&) noexcept = delete;
	GfxImage(GfxImage&& other) noexcept = default;
	GfxImage& operator=(GfxImage&& other) noexcept = default;

	[[nodiscard]] bool IsSampledImage() const;
	[[nodiscard]] bool IsStorageImage() const;
	[[nodiscard]] bool IsColorAttachment() const;
	[[nodiscard]] bool IsDepthAttachment() const;
	[[nodiscard]] bool IsAttachment() const;

	[[nodiscard]] static StageAccess GetPipelineStageAccess(VkImageLayout layout);

	static void ImageMemoryBarrier(VkCommandBuffer cmdBuffer,
		VkImage image,
		StageAccess src,
		StageAccess dst,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkImageSubresourceRange subresourceRange);

	[[nodiscard]] VkImageView CreateImageView(VkImageViewType type,
		VkFormat format,
		VkImageAspectFlags aspectMask,
		uint32_t baseLevel,
		uint32_t numLevels = VK_REMAINING_MIP_LEVELS,
		uint32_t baseLayer = 0,
		uint32_t numLayers = 1,
		const VkComponentMapping mapping = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
											 .g = VK_COMPONENT_SWIZZLE_IDENTITY,
											 .b = VK_COMPONENT_SWIZZLE_IDENTITY,
											 .a = VK_COMPONENT_SWIZZLE_IDENTITY },
		const char* debugName = nullptr) const;

	void GenerateMipmap(VkCommandBuffer cmdBuffer) const;
	void TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newImageLayout, const VkImageSubresourceRange& subresourceRange) const;

	[[nodiscard]] VkImageAspectFlags GetImageAspectFlags() const;

	// Framebuffers can render only into one level/layer
	[[nodiscard]] VkImageView GetOrCreateVkImageViewForFramebuffer(uint8_t level, uint16_t layer);

	[[nodiscard]] static bool IsDepthFormat(VkFormat format);
	[[nodiscard]] static bool IsStencilFormat(VkFormat format);


	VkImage m_VkImage{ VK_NULL_HANDLE };
	VkImageUsageFlags m_VkUsageFlags{};
	VkDeviceMemory m_VkMemory{ VK_NULL_HANDLE };
	//VmaAllocation m_VmaAllocation{ VK_NULL_HANDLE };
	VkFormatProperties m_VkFormatProperties{};
	VkExtent3D m_VkExtent{ 0, 0, 0 };
	VkImageType m_VkType{ VK_IMAGE_TYPE_MAX_ENUM };
	VkFormat m_VkImageFormat{ VK_FORMAT_UNDEFINED };
	VkSampleCountFlagBits m_VkSamples{ VK_SAMPLE_COUNT_1_BIT };
	void* m_MappedPtr{};
	bool m_IsSwapchainImage{};
	bool m_IsOwningVkImage{ true };
	bool m_IsResolveAttachment{}; // autoset by cmdBeginRendering() for extra synchronization
	uint32_t m_NumLevels{ 1 };
	uint32_t m_NumLayers{ 1 };
	bool m_IsDepthFormat{};
	bool m_IsStencilFormat{};
	char m_DebugName[256]{};
	mutable VkImageLayout m_CurrentVkImageLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
	// precached image views - owned by this VulkanImage
	static constexpr uint32_t sk_MaxMipLevels{ 16 };
	static constexpr uint32_t sk_MaxArrayLayers{ 6 }; // max 6 faces for cubemap rendering
	VkImageView m_ImageView{ VK_NULL_HANDLE }; // default view with all mip-levels
	VkImageView m_ImageViewStorage{ VK_NULL_HANDLE }; // default view with identity swizzle (all mip-levels)
	VkImageView m_ImageViewForFramebuffer[sk_MaxMipLevels][sk_MaxArrayLayers]{};
	GraphicsAPI* m_pGraphicsAPI;
};

#pragma endregion

#endif //GFXSTRUCTS_H