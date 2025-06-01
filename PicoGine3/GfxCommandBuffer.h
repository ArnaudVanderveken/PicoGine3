#ifndef GFXCOMMANDBUFFER_H
#define GFXCOMMANDBUFFER_H

class GfxImmediateCommands;
struct CommandBufferWrapper;

class GfxCommandBuffer final
{
public:
	explicit GfxCommandBuffer() = default;
	explicit GfxCommandBuffer(GfxImmediateCommands* pGfxImmediateCommands);
	~GfxCommandBuffer() = default;

	GfxCommandBuffer(const GfxCommandBuffer&) noexcept = delete;
	GfxCommandBuffer& operator=(const GfxCommandBuffer&) noexcept = delete;
	GfxCommandBuffer(GfxCommandBuffer&&) noexcept = default;
	GfxCommandBuffer& operator=(GfxCommandBuffer&&) noexcept = default;

	[[nodiscard]] const CommandBufferWrapper* GetBufferWrapper() const;
	[[nodiscard]] VkCommandBuffer GetCmdBuffer() const;

private:
	GfxImmediateCommands* m_pGfxImmediateCommands{};
	const CommandBufferWrapper* m_pWrapper{};
};

#endif //GFXCOMMANDBUFFER_H