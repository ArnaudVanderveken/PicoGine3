#ifndef GFXSWAPCHAIN_H
#define GFXSWAPCHAIN_H
#include "GfxImage.h"

#if defined(_DX)

class GfxSwapchain final
{
public:
	explicit GfxSwapchain(GfxDevice* device) = default;
	~GfxSwapchain() = default;

	GfxSwapchain(const GfxSwapchain&) noexcept = delete;
	GfxSwapchain& operator=(const GfxSwapchain&) noexcept = delete;
	GfxSwapchain(GfxSwapchain&&) noexcept = delete;
	GfxSwapchain& operator=(GfxSwapchain&&) noexcept = delete;

private:


};

#elif defined(_VK)

class GraphicsAPI;
class GfxDevice;

class GfxSwapchain final
{
public:
	explicit GfxSwapchain(GraphicsAPI* pGraphicsAPI);
	~GfxSwapchain();

	GfxSwapchain(const GfxSwapchain&) noexcept = delete;
	GfxSwapchain& operator=(const GfxSwapchain&) noexcept = delete;
	GfxSwapchain(GfxSwapchain&&) noexcept = delete;
	GfxSwapchain& operator=(GfxSwapchain&&) noexcept = delete;

	[[nodiscard]] VkFramebuffer GetCurrentFrameBuffer() const;
	[[nodiscard]] VkRenderPass GetRenderPass() const;
	[[nodiscard]] VkImageView GetImageView(int index) const;
	[[nodiscard]] size_t GetImageCount() const;
	[[nodiscard]] VkFormat GetSwapChainImageFormat() const;
	[[nodiscard]] VkExtent2D GetSwapChainExtent() const;
	[[nodiscard]] uint32_t Width() const;
	[[nodiscard]] uint32_t Height() const;
	[[nodiscard]] float AspectRatio() const;
	[[nodiscard]] uint64_t GetCurrentFrameIndex() const;

	[[nodiscard]] VkFormat FindDepthFormat() const;

	void SetCurrentFrameTimelineWaitValue(uint64_t value);
	[[nodiscard]] VkResult Present(VkSemaphore semaphore);

	void RecreateSwapchain();
	VkResult AcquireImage();

	static constexpr uint32_t sk_MaxFramesInFlight{ 3 };

private:
	GraphicsAPI* m_pGraphicsAPI;

	VkFormat m_VkSwapChainColorFormat;
	VkFormat m_VkSwapChainDepthFormat;
	VkExtent2D m_VkSwapChainExtent;

	std::vector<VkFramebuffer> m_VkFrameBuffers;
	VkRenderPass m_VkRenderPass{ VK_NULL_HANDLE };

	VkSwapchainKHR m_VkSwapChain{ VK_NULL_HANDLE };
	std::vector<GfxImage> m_SwapChainImages;
	std::vector<GfxImage> m_DepthImages;

	std::array<VkSemaphore, sk_MaxFramesInFlight> m_AcquireSemaphores;
	std::array<uint64_t, sk_MaxFramesInFlight> m_TimelineWaitValues;

	uint64_t m_CurrentFrame{};
	uint32_t m_CurrentFrameSwapchainImageIndex{};

	bool m_GetNextImage{ true };

	static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	void CreateSwapchain();
	void CreateDepthResources();
	void CreateFrameBuffers();
	void CleanupSwapchain();
	void CreateRenderPass();
	void CreateSyncObjects();
};

#endif

#endif //GFXSWAPCHAIN_H