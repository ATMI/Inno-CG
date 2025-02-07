#pragma once

#include "utils/error_handler.h"

#include <algorithm>
#include <linalg.h>
#include <vector>


using namespace linalg::aliases;

namespace cg
{
	template<typename T>
	class resource
	{
	public:
		explicit resource(size_t size);
		resource(size_t x_size, size_t y_size);
		~resource();

		const T* get_data();
		T& item(size_t item);
		T& item(size_t x, size_t y);

		size_t size_bytes() const;
		size_t count() const;
		size_t get_stride() const;

	private:
		std::vector<T> data;
		size_t item_size = sizeof(T);
		size_t stride;
	};

	template<typename T>
	inline resource<T>::resource(size_t size)
		: stride(0),
		  data(size)
	{
	}

	template<typename T>
	inline resource<T>::resource(size_t x_size, size_t y_size)
		: stride(x_size),
		  data(x_size * y_size)
	{
	}

	template<typename T>
	inline resource<T>::~resource() = default;

	template<typename T>
	inline const T* resource<T>::get_data()
	{
		return data.data();
	}

	template<typename T>
	inline T& resource<T>::item(size_t item)
	{
		return data[item];
	}

	template<typename T>
	inline T& resource<T>::item(size_t x, size_t y)
	{
		return data[stride * y + x];
	}

	template<typename T>
	inline size_t resource<T>::size_bytes() const
	{
		return item_size * count();
	}

	template<typename T>
	inline size_t resource<T>::count() const
	{
		return data.size();
	}

	template<typename T>
	inline size_t resource<T>::get_stride() const
	{
		return stride;
	}

	struct color
	{
		static color from_float3(const float3& in)
		{
			return color{in.x, in.y, in.z};
		};

		float3 to_float3() const
		{
			return float3{r, g, b};
		}

		float r;
		float g;
		float b;
	};

	struct unsigned_color
	{
		static unsigned_color from_color(const color& color)
		{
			unsigned_color res{};
			res.r = std::clamp(int(color.r * 255.f), 0, 255);
			res.g = std::clamp(int(color.g * 255.f), 0, 255);
			res.b = std::clamp(int(color.b * 255.f), 0, 255);
			return res;
		};

		static unsigned_color from_float3(const float3& color)
		{
			unsigned_color res{};
			float3 converted = clamp(color * 255.f, 0.f, 255.f);
			res.r = int(converted.x);
			res.g = int(converted.y);
			res.b = int(converted.z);
			return res;
		};

		float3 to_float3() const
		{
			return {float(r), float(g), float(b)};
		};

		uint8_t r;
		uint8_t g;
		uint8_t b;
	};


	struct vertex
	{
		float3 position;
		float3 normal;
		float2 texture;
		float3 ambient;
		float3 diffuse;
		float3 emissive;
	};

}// namespace cg