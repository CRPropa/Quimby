#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cmath>
#include <limits>

#include <stdint.h>

#include "quimby/tga.h"

namespace quimby {

void tga_write_header(std::ostream &o, size_t width, size_t height) {
	o.put(0);
	o.put(0);
	o.put(2); /* uncompressed RGB */
	o.put(0);
	o.put(0);
	o.put(0);
	o.put(0);
	o.put(0);
	o.put(0);
	o.put(0); /* X origin */
	o.put(0);
	o.put(0); /* y origin */
	o.put((width & 0x00FF));
	o.put((width & 0xFF00) / 256);
	o.put((height & 0x00FF));
	o.put((height & 0xFF00) / 256);
	o.put(32); /* 24 bit bitmap */
	o.put(0);
}

#define rgb(r, g, b) ((255ul << 24) | (r<<16) | (g << 8) | (b << 0) )
#define rgba(r, g, b, a) ((a << 24) | (r<<16) | (g << 8) | (b << 0) )

uint32_t cmap_hot[] = { 0xFF000000, 0xFFFF0000, 0xFFFF8800, 0xFFFFFF00,
		0xFFFFFFFF };
uint32_t cmap_gray[] = { 0xFF000000, 0xFFFFFFFF };

uint32_t cmap_blue[] = { rgb(241, 238, 246), rgb(189, 201, 225), rgb(116, 169,
		207), rgb(43, 140, 190), rgb(4, 90, 141) };

uint32_t cmap_warm[] = { rgb(255, 255, 178), rgb(254, 204, 92), rgb(253, 141,
		60), rgb(240, 59, 32), rgb(189, 0, 38) };

uint32_t cmap_parula[] = { rgb(53, 42, 135), rgb(54, 48, 147), rgb(54, 55, 160),
		rgb(53, 61, 173), rgb(50, 67, 186), rgb(44, 74, 199), rgb(32, 83, 212),
		rgb(15, 92, 221), rgb(3, 99, 225), rgb(2, 104, 225), rgb(4, 109, 224),
		rgb(8, 113, 222), rgb(13, 117, 220), rgb(16, 121, 218), rgb(18, 125,
				216), rgb(20, 129, 214), rgb(20, 133, 212), rgb(19, 137, 211),
		rgb(16, 142, 210), rgb(12, 147, 210), rgb(9, 152, 209), rgb(7, 156,
				207), rgb(6, 160, 205), rgb(6, 164, 202), rgb(6, 167, 198), rgb(
				7, 169, 194), rgb(10, 172, 190), rgb(15, 174, 185), rgb(21, 177,
				180), rgb(29, 179, 175), rgb(37, 181, 169), rgb(46, 183, 164),
		rgb(56, 185, 158), rgb(66, 187, 152), rgb(77, 188, 146), rgb(89, 189,
				140), rgb(101, 190, 134), rgb(113, 191, 128), rgb(124, 191,
				123), rgb(135, 191, 119), rgb(146, 191, 115), rgb(156, 191,
				111), rgb(165, 190, 107), rgb(174, 190, 103), rgb(183, 189,
				100), rgb(192, 188, 96), rgb(200, 188, 93), rgb(209, 187, 89),
		rgb(217, 186, 86), rgb(225, 185, 82), rgb(233, 185, 78), rgb(241, 185,
				74), rgb(248, 187, 68), rgb(253, 190, 61), rgb(255, 195, 55),
		rgb(254, 200, 50), rgb(252, 206, 46), rgb(250, 211, 42), rgb(247, 216,
				38), rgb(245, 222, 33), rgb(245, 228, 29), rgb(245, 235, 24),
		rgb(246, 243, 19), rgb(249, 251, 14) };

void tga_write_float(std::ostream &o, float f) {
	tga_write_float_cmap(o, f, cmap_parula,
			sizeof(cmap_parula) / sizeof(*cmap_parula));
}

uint32_t interpolate(uint32_t a, uint32_t b, float f) {
	uint32_t c;
	uint8_t *ca = (uint8_t *) &a;
	uint8_t *cb = (uint8_t *) &b;
	uint8_t *cc = (uint8_t *) &c;
	for (size_t i = 0; i < 4; i++) {
		cc[i] = ca[i] + (cb[i] - ca[i]) * f;
	}
	return c;
}

#define WRITE_COLOR(o, a) o.write((const char *)&a, 4);

void tga_write_float_cmap(std::ostream &o, float f, const uint32_t *cmap,
		const size_t cmap_size) {
	uint32_t c = 0;
	if (f == -std::numeric_limits<float>::infinity()) {
		c = rgba(0, 0, 0, 0);
	} else if (f == std::numeric_limits<float>::infinity()) {
		c = rgba(0, 0, 0, 0);
	} else if (f <= 0) {
		c = cmap[0];
	} else if (f >= 1) {
		c = cmap[cmap_size - 1];
	} else if (f > 0 && f < 1) {
		size_t idx = f * (cmap_size - 1);
		float step = 1. / (cmap_size - 1);
		float rf = fmod(f, step) / step;
		c = interpolate(cmap[idx], cmap[idx + 1], rf);
	}
	WRITE_COLOR(o, c);
}

void tga_write_float_cmap(std::ostream &o, float f,
		const std::vector<uint32_t> &cmap) {
	tga_write_float_cmap(o, f, cmap.data(), cmap.size());
}

void tga_write(const std::string &filename, float *data, size_t width,
		size_t height) {
	std::ofstream out(filename.c_str(), std::ios::binary);
	tga_write_header(out, width, height);
	for (size_t i = 0; i < (width * height); i++)
		tga_write_float(out, data[i]);
}

} // namespace
