/*
 * transpose.h
 *
 *  Created on: 14.03.2011
 *      Author: gmueller
 */

#include "gadget/transpose.h"
#include <memory.h>

#define TRANSPOSE_B(a) ((src[stride##a ] & mask) >> bit << a)

typedef unsigned char transpose_byte_t;

void transpose_forward(const void *input, void *output, size_t element_count,
		size_t element_size, size_t stride) {
	const transpose_byte_t *in = (transpose_byte_t *) input;
	transpose_byte_t *out = (transpose_byte_t *) output;
	size_t count = (element_count / 8) * 8;

	size_t bitsPerElement = element_size * 8;
	size_t bytesPerRow = count / 8;

	size_t iBit = 0;
	for (iBit = 0; iBit < bitsPerElement; iBit++) {
		size_t stride0 = 0;
		size_t stride1 = stride;
		size_t stride2 = stride * 2;
		size_t stride3 = stride * 3;
		size_t stride4 = stride * 4;
		size_t stride5 = stride * 5;
		size_t stride6 = stride * 6;
		size_t stride7 = stride * 7;
		size_t stride8 = stride * 8;

		size_t byteOffset = iBit / 8;
		size_t bit = iBit % 8;
		transpose_byte_t *byte = out + iBit * bytesPerRow;
		size_t iElement, iB;
		transpose_byte_t mask = (1 << bit);
		const transpose_byte_t *src = (const transpose_byte_t*) (in
				+ byteOffset);
		for (iElement = 0, iB = 0; iElement < count; iElement += 8, iB++) {
			transpose_byte_t b = TRANSPOSE_B(0) | TRANSPOSE_B(1)
					| TRANSPOSE_B(2) | TRANSPOSE_B(3) | TRANSPOSE_B(4)
					| TRANSPOSE_B(5) | TRANSPOSE_B(6) | TRANSPOSE_B(7);
			byte[iB] = b;
			src += stride8;
		}
	}

	// copy the remaining elements
	size_t i;
	for (i = count; i < element_count; i++) {
		memcpy(out + element_size * i, in + stride * i, element_size);
	}
}

void transpose_backward(const void *input, void *output, size_t element_count,
		size_t element_size, size_t stride) {
	const transpose_byte_t *in = (transpose_byte_t *) input;
	transpose_byte_t *out = (transpose_byte_t *) output;
	size_t count = (element_count / 8) * 8;
	size_t bytesPerRow = count / 8;

	size_t iE, iByte;
	for (iE = 0; iE < count; iE++) {
		size_t targetOffset = iE * stride;
		size_t srcByteOffset0 = iE / 8;
		size_t srcByteOffset1 = srcByteOffset0 + bytesPerRow;
		size_t srcByteOffset2 = srcByteOffset1 + bytesPerRow;
		size_t srcByteOffset3 = srcByteOffset2 + bytesPerRow;
		size_t srcByteOffset4 = srcByteOffset3 + bytesPerRow;
		size_t srcByteOffset5 = srcByteOffset4 + bytesPerRow;
		size_t srcByteOffset6 = srcByteOffset5 + bytesPerRow;
		size_t srcByteOffset7 = srcByteOffset6 + bytesPerRow;
		size_t srcBit = iE % 8;
		transpose_byte_t mask = (1 << srcBit);
		for (iByte = 0; iByte < element_size; iByte++) {
			transpose_byte_t b;
			size_t offset2 = iByte * bytesPerRow * 8;
			b = (in[srcByteOffset0 + offset2] & mask) >> srcBit << 0;
			b |= (in[srcByteOffset1 + offset2] & mask) >> srcBit << 1;
			b |= (in[srcByteOffset2 + offset2] & mask) >> srcBit << 2;
			b |= (in[srcByteOffset3 + offset2] & mask) >> srcBit << 3;
			b |= (in[srcByteOffset4 + offset2] & mask) >> srcBit << 4;
			b |= (in[srcByteOffset5 + offset2] & mask) >> srcBit << 5;
			b |= (in[srcByteOffset6 + offset2] & mask) >> srcBit << 6;
			b |= (in[srcByteOffset7 + offset2] & mask) >> srcBit << 7;
			out[targetOffset + iByte] = b;
		}
	}

	// copy the remaining elements
	size_t i = 0;
	for (i = count; i < element_count; i++) {
		memcpy(out + stride * i, in + element_size * i, element_size);
	}

}
