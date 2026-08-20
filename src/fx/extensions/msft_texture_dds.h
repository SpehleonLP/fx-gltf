#ifndef MSFT_TEXTURE_DDS_H
#define MSFT_TEXTURE_DDS_H
#include <cstdint>

namespace MSFT
{

struct texture_dds
{
	int32_t source{-1};
	uint32_t uncompressedSize{};

	bool empty() const { return source == -1; }
	bool operator==(texture_dds const& it) const
	{
		return source == it.source && uncompressedSize == it.uncompressedSize;
	}
};

}

namespace LF
{

enum class SWIZZLE : char
{
	zero,
	one,
	red,
	green,
	blue,
	alpha
};

struct Swizzle
{
	Swizzle() :
		r(SWIZZLE::red),
		g(SWIZZLE::green),
		b(SWIZZLE::blue),
		a(SWIZZLE::alpha)
	{
	}

	SWIZZLE GetChannel(SWIZZLE in) const
	{
		if((int)in <= 1)
			return in;

		switch(in)
		{
		case SWIZZLE::zero:
		case SWIZZLE::one:		return in;
		case SWIZZLE::red:		return r;
		case SWIZZLE::green:	return g;
		case SWIZZLE::blue:		return b;
		case SWIZZLE::alpha:	return a;
		default:
			return in;
		}

		return in;
	}

	Swizzle swizzle(Swizzle it) const
	{
		Swizzle r;

		r.r = GetChannel(it.r);
		r.b = GetChannel(it.b);
		r.g = GetChannel(it.g);
		r.a = GetChannel(it.a);

		return r;
	}

	bool       empty() const
	{
		return r == SWIZZLE::red
			&& g == SWIZZLE::green
			&& b == SWIZZLE::blue
			&& a == SWIZZLE::alpha;
	}

	void GetMask(int * dst) const
	{
		const short converter[] = { 0, 1, 0x1903, 0x1904, 0x1905, 0x1906 };

		dst[0] = converter[(int)r];
		dst[1] = converter[(int)b];
		dst[2] = converter[(int)g];
		dst[3] = converter[(int)a];
	}

	SWIZZLE r;
	SWIZZLE g;
	SWIZZLE b;
	SWIZZLE a;

	bool operator==(Swizzle const& it) const { return r == it.r && g == it.g && b == it.b && a == it.a; }
};

struct Sampler
{
	uint16_t   magFilter{9729};
	uint16_t   minFilter{9729};
	uint16_t   wrapS{10497};
	uint16_t   wrapT{10497};
	uint16_t   wrapR{10497};
	Swizzle    swizzle;

	void ApplySettings(uint32_t target, uint32_t noMipMaps) const;
};

struct texture_cmp
{
	short      bc{-1};
	Swizzle swizzle{};

	bool empty() const
	{
		return bc == -1
			&& swizzle.empty();
	}

	bool operator==(texture_cmp const& it) const { return bc == it.bc && swizzle == it.swizzle; }
};


}


#endif // MSFT_TEXTURE_DDS_H
