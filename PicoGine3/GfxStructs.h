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

#pragma endregion

#pragma region GfxBuffer

struct GfxBuffer final
{
public:
	explicit GfxBuffer(GraphicsAPI* pGraphicsAPI, size_t elementStride, size_t elementCount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, size_t minAlignmentOffset = 0);
	~GfxBuffer();

	GfxBuffer(const GfxBuffer& other) noexcept = delete;
	GfxBuffer& operator=(const GfxBuffer& other) noexcept = delete;
	GfxBuffer(GfxBuffer&& other) noexcept = delete;
	GfxBuffer& operator=(GfxBuffer&& other) noexcept = delete;

	void Map(size_t size = ~0ULL, size_t offset = 0);
	void Unmap();

	[[nodiscard]] size_t GetBufferSize() const;

	void WriteToBuffer(const void* data, size_t size = ~0ULL, size_t offset = 0) const;
	void Flush(size_t size = ~0ULL, size_t offset = 0) const;
	void Invalidate(size_t size = ~0ULL, size_t offset = 0) const;

	void WriteToIndex(const void* data, int index) const;
	void FlushIndex(int index) const;
	void InvalidateIndex(int index) const;

	[[nodiscard]] VkBuffer GetBuffer() const;
	[[nodiscard]] VkDeviceMemory GetBufferMemory() const;

private:
	GraphicsAPI* m_pGraphicsAPI;

	VkBuffer m_Buffer;
	VkDeviceMemory m_BufferMemory;
	void* m_pMappedMemory;
	VkDeviceAddress m_VkDeviceAddress;
	VkBufferUsageFlags m_UsageFlags;
	VkMemoryPropertyFlags m_MemoryFlags;

	size_t m_ElementStride;
	size_t m_ElementCount;
	size_t m_AlignmentSize;
	size_t m_BufferSize;

	static size_t GetAlignment(size_t elementStride, size_t minAlignmentOffset);
};

#pragma endregion

#pragma region GfxImage

struct StageAccess
{
	VkPipelineStageFlags2 m_Stage;
	VkAccessFlags2 m_Access;
};

struct GfxImage final
{
	explicit GfxImage(GraphicsAPI* pGraphicsAPI);
	~GfxImage();

	GfxImage(const GfxImage&) noexcept = delete;
	GfxImage& operator=(const GfxImage&) noexcept = delete;
	GfxImage(GfxImage&& other) noexcept;
	GfxImage& operator=(GfxImage&& other) noexcept;

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
		const VkSamplerYcbcrConversionInfo* ycbcr = nullptr,
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
	VkDeviceMemory m_VkMemory[3]{ VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
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

private:
	GraphicsAPI* m_pGraphicsAPI;
};

#pragma endregion

#endif //GFXSTRUCTS_H