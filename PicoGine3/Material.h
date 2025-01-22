#ifndef MATERIAL_H
#define MATERIAL_H


class Material
{
public:
	explicit Material() = default;
	virtual ~Material() = default;

	Material(const Material&) noexcept = delete;
	Material& operator=(const Material&) noexcept = delete;
	Material(Material&&) noexcept = delete;
	Material& operator=(Material&&) noexcept = delete;

private:
	

};


#endif //MATERIAL_H