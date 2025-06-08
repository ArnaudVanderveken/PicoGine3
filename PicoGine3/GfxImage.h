#ifndef GFXIMAGE_H
#define GFXIMAGE_H


class GfxDevice;
class GraphicsAPI;

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

#endif //GFXIMAGE_H