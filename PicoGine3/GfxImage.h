#ifndef GFXIMAGE_H
#define GFXIMAGE_H


class GfxImage final
{
public:
	explicit GfxImage() = default;
	~GfxImage() = default;

	GfxImage(const GfxImage&) noexcept = delete;
	GfxImage& operator=(const GfxImage&) noexcept = delete;
	GfxImage(GfxImage&&) noexcept = delete;
	GfxImage& operator=(GfxImage&&) noexcept = delete;

private:
	

};

#endif //GFXIMAGE_H