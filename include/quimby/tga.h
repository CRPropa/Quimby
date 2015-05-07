#pragma once

namespace quimby {

void tga_write_header(std::ostream &o, size_t width, size_t height);
void tga_write_float(std::ostream &o, float f);
	void tga_write_float_cmap(std::ostream &o, float f,
			const uint32_t *cmap, const size_t cmap_size);
void tga_write_float_cmap(std::ostream &o, float f,
		const std::vector<uint32_t> &cmap);
void tga_write(const std::string &filename, float *data, size_t width,
		size_t height);

} // namespace

