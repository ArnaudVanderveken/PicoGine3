#ifndef GFXSWAPCHAIN_H
#define GFXSWAPCHAIN_H

class GfxSwapchain final
{
public:
	explicit GfxSwapchain() = default;
	~GfxSwapchain() = default;

	GfxSwapchain(const GfxSwapchain&) noexcept = delete;
	GfxSwapchain& operator=(const GfxSwapchain&) noexcept = delete;
	GfxSwapchain(GfxSwapchain&&) noexcept = delete;
	GfxSwapchain& operator=(GfxSwapchain&&) noexcept = delete;

private:
	VkSwapchainKHR m_VkSwapChain;
	std::vector<VkImage> m_VkSwapChainImages;
	std::vector<VkImageView> m_VkSwapChainImageViews;
	VkFormat m_VkSwapChainImageFormat;
	VkExtent2D m_VkSwapChainExtent;
	std::vector<VkFramebuffer> m_VkFrameBuffers;
};

#endif //GFXSWAPCHAIN_H