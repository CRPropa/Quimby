#ifndef _GADGET_KERNEL_H_
#define _GADGET_KERNEL_H_

namespace gadget {

inline float kernel(float r) {
	if (r < 1.0) {
		return 1.0 - 1.5 * r * r + 0.75 * r * r * r;
	} else if (r < 2.0) {
		float x = (2.0 - r);
		return 0.25 * x * x * x;
	} else {
		return 0.0;
	}
}

inline float kernel(float value, float center, float position, float hsml) {
	return value * kernel(std::fabs((center - position) / hsml));
}

} // namespace gadget

#endif /* _GADGET_KERNEL_H_ */
