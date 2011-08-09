/*
 * transpose.h
 *
 *  Created on: 14.03.2011
 *      Author: gmueller
 */

#ifndef TRANSPOSE_H_
#define TRANSPOSE_H_

#if defined (__cplusplus)
extern "C" {
#endif

#include <stddef.h>

void transpose_forward(const void *input, void *output, size_t element_count,
		size_t element_size, size_t stride);

void transpose_backward(const void *input, void *output, size_t element_count,
		size_t element_size, size_t stride);

#if defined (__cplusplus)
}
#endif

#endif /* TRANSPOSE_H_ */
