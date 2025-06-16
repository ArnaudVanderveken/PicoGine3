#ifndef GFXRENDERPIPELINE_H
#define GFXRENDERPIPELINE_H

class GfxRenderPipeline final
{
public:
	explicit GfxRenderPipeline() = default;
	virtual ~GfxRenderPipeline() = default;

	GfxRenderPipeline(const GfxRenderPipeline&) noexcept = delete;
	GfxRenderPipeline& operator=(const GfxRenderPipeline&) noexcept = delete;
	GfxRenderPipeline(GfxRenderPipeline&&) noexcept = delete;
	GfxRenderPipeline& operator=(GfxRenderPipeline&&) noexcept = delete;

private:
	

};

#endif //GFXRENDERPIPELINE_H