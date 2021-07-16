#pragma once

#ifndef ZIMG_GRAPH_IMAGE_BUFFER_H_
#define ZIMG_GRAPH_IMAGE_BUFFER_H_

#include <cstddef>
#include <limits>
#include <type_traits>

#ifdef _MSC_VER
  #include <intrin.h>
#endif

namespace zimg {
namespace graph {

// Special mask value with all bits set.
constexpr unsigned BUFFER_MAX = ~0U;

/**
 * Circular image buffer.
 *
 * @tparam T held type
 */
template <class T>
class ImageBuffer {
	typedef std::conditional_t<std::is_const<T>::value, const void, void> void_pointer;
	typedef std::conditional_t<std::is_const<T>::value, const char, char> char_pointer;

	// Use a void pointer to ensure all instantiations are layout compatible.
	void_pointer *m_data;
	ptrdiff_t m_stride;
	unsigned m_mask;

	T *at_line(unsigned i) const noexcept
	{
		char_pointer *data = static_cast<char_pointer *>(m_data);
		return reinterpret_cast<T *>(data + static_cast<ptrdiff_t>(i & m_mask) * m_stride);
	}
public:
	/**
	 * Default construct ImageBuffer, creating a null buffer.
	 */
	constexpr ImageBuffer() noexcept : m_data{}, m_stride{}, m_mask{} {}

	/**
	 * Construct an ImageBuffer from pointer and mask.
	 *
	 * @param data pointer to base of buffer
	 * @param stride buffer stride in bytes, may be negative
	 * @param mask row index mask
	 */
	constexpr ImageBuffer(T *data, ptrdiff_t stride, unsigned mask) noexcept :
		m_data{ data },
		m_stride{ stride },
		m_mask{ mask }
	{
		static_assert(std::is_standard_layout<ImageBuffer>::value, "layout error");
	}

	/**
	 * Construct an ImageBuffer from a buffer of an implicitly convertible type.
	 *
	 * @tparam U type convertible to T
	 * @param other original buffer
	 */
	template <class U>
	constexpr ImageBuffer(const ImageBuffer<U> &other,
	                      std::enable_if_t<std::is_convertible<U *, T *>::value> * = nullptr) noexcept :
		ImageBuffer{ other.data(), other.stride(), other.mask() }
	{}

	/**
	 * Get the underlying data pointer.
	 *
	 * @return pointer
	 */
	T *data() const noexcept { return at_line(0); }

	/**
	 * Get the underlying image stride.
	 *
	 * @return stride
	 */
	ptrdiff_t stride() const noexcept { return m_stride; }

	/**
	 * Get the underlying row mask.
	 *
	 * @return mask
	 */
	unsigned mask() const noexcept { return m_mask; }

	/**
	 * Get pointer to scanline.
	 *
	 * @param i row index
	 * @return pointer to beginning of line
	 */
	T *operator[](unsigned i) const noexcept { return at_line(i); }

	/**
	 * Cast buffer to another type.
	 *
	 * @tparam U type convertible from T by static_cast
	 * @return buffer as other type
	 */
	template <class U>
	const ImageBuffer<U> &static_buffer_cast() const noexcept
	{
		static_assert(std::is_standard_layout<decltype(static_cast<U *>(static_cast<T *>(nullptr)))>::value,
		              "type not convertible by static_cast");

		// Break strict aliasing to avoid unnecessary object copies.
		return *reinterpret_cast<const ImageBuffer<U> *>(this);
	}
};

/**
 * Wrapper around array of four {@link ImageBuffer}.
 *
 * @tparam T buffer held type
 */
template <class T>
class ColorImageBuffer {
	ImageBuffer<T> m_buffer[4];
public:
	/**
	 * Default construct ColorImageBuffer, creating an array of null buffers.
	 */
	constexpr ColorImageBuffer() noexcept : m_buffer{} {}

	/**
	 * Construct a ColorImageBuffer from individual buffers.
	 *
	 * @param buf1 first channel
	 * @param buf2 second channel
	 * @param buf3 third channel
	 */
	constexpr ColorImageBuffer(const ImageBuffer<T> &buf1, const ImageBuffer<T> &buf2, const ImageBuffer<T> &buf3) noexcept :
		m_buffer{ buf1, buf2, buf3 }
	{}

	/**
	 * Construct a ColorImageBuffer from individual buffers.
	 *
	 * @param buf1 first channel
	 * @param buf2 second channel
	 * @param buf3 third channel
	 * @param buf4 fourth channel
	 */
	constexpr ColorImageBuffer(const ImageBuffer<T> &buf1, const ImageBuffer<T> &buf2, const ImageBuffer<T> &buf3, const ImageBuffer<T> &buf4) noexcept :
		m_buffer{ buf1, buf2, buf3, buf4 }
	{}

	/**
	 * Construct a ColorImageBuffer from an implicitly convertible type.
	 *
	 * @tparam U type convertible to T
	 * @param other original buffer
	 */
	template <class U>
	constexpr ColorImageBuffer(const ColorImageBuffer<U> &other,
	                           std::enable_if_t<std::is_convertible<U *, T *>::value> * = nullptr) noexcept :
		ColorImageBuffer{ other[0], other[1], other[2], other[3] }
	{}

	/**
	 * Implicit conversion to array of {@link ImageBuffer}.
	 *
	 * @return pointer
	 */
	operator const ImageBuffer<T> *() const noexcept { return m_buffer; }

	/**
	 * @see operator const ImageBuffer<T> *
	 */
	operator ImageBuffer<T> *() noexcept { return m_buffer; }

	/**
	* Cast buffer to another type.
	*
	* @tparam U type convertible from T by static_cast
	* @return buffer as other type
	*/
	template <class U>
	const ColorImageBuffer<U> &static_buffer_cast() const noexcept
	{
		static_assert(std::is_standard_layout<decltype(static_cast<U *>(static_cast<T *>(nullptr)))>::value,
		              "type not convertible by static_cast");

		// Break strict aliasing to avoid unnecessary object copies.
		return *reinterpret_cast<const ColorImageBuffer<U> *>(this);
	}
};

/**
 * @see ImageBuffer::static_buffer_cast
 */
template <class U, class T>
const ImageBuffer<U> &static_buffer_cast(const ImageBuffer<T> &buf) noexcept
{
	return buf.template static_buffer_cast<U>();
}

/**
 * @see ImageBuffer::static_buffer_cast
 */
template <class U, class T>
const ImageBuffer<U> *static_buffer_cast(const ImageBuffer<T> *buf) noexcept
{
	return &static_buffer_cast<U>(*buf);
}

/**
 * @see ColorImageBuffer::static_buffer_cast
 */
template <class U, class T>
const ColorImageBuffer<U> &static_buffer_cast(const ColorImageBuffer<T> &buf) noexcept
{
	return buf.template static_buffer_cast<U>();
}

/**
 * @see ColorImageBuffer::static_buffer_cast
 */
template <class U, class T>
const ColorImageBuffer<U> *static_buffer_cast(const ColorImageBuffer<T> *buf) noexcept
{
	return &static_buffer_cast<U>(*buf);
}

/**
 * Convert a line count to a buffer mask.
 *
 * @param count line count, may be {@link BUFFER_MAX}
 * @return mask, may be {@link BUFFER_MAX}
 */
inline unsigned select_zimg_buffer_mask(unsigned count) noexcept
{
	unsigned long lzcnt;

	if (count <= 1)
		return 0;

#if defined(_MSC_VER)
	unsigned long msb;
	_BitScanReverse(&msb, count - 1);
	lzcnt = 31 - msb;
#elif defined(__GNUC__)
	lzcnt = __builtin_clz(count - 1);
#else
	lzcnt = 0;
	count -= 1;
	while (!(count & (1U << (std::numeric_limits<unsigned>::digits - 1)))) {
		count <<= 1;
		++lzcnt;
	}
#endif

	return BUFFER_MAX >> lzcnt;
}

} // namespace graph
} // namespace zimg

#endif // ZIMG_GRAPH_ZTYPES_H_
