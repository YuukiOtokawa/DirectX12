#pragma once

#include <math.h>

struct Vector2 {
	float x, y;

	Vector2() : x(0), y(0) {}
	Vector2(float x, float y) : x(x), y(y) {}
	Vector2(float value) : x(value), y(value) {}

	Vector2 operator+(const Vector2& other) const {
		return Vector2(x + other.x, y + other.y);
	}
	Vector2 operator+(const float scalar) const {
		return Vector2(x + scalar, y + scalar);
	}
	Vector2 operator-(const Vector2& other) const {
		return Vector2(x - other.x, y - other.y);
	}
	Vector2 operator-(const float scalar) const {
		return Vector2(x - scalar, y - scalar);
	}
	Vector2 operator*(float scalar) const {
		return Vector2(x * scalar, y * scalar);
	}
	Vector2 operator/(float scalar) const {
		return Vector2(x / scalar, y / scalar);
	}
	bool operator==(const Vector2& other) const {
		return x == other.x && y == other.y;
	}


	float Length() const {
		return sqrt(x * x + y * y);
	}
	float Dot(const Vector2& other) const {
		return x * other.x + y * other.y;
	}
	Vector2 Normalize() const {
		float length = Length();
		if (length == 0) return Vector2(0, 0);
		return Vector2(x / length, y / length);
	}
	static Vector2 Lerp(const Vector2& a, const Vector2& b, float t) {
		return a + (b - a) * t;
	}

};

struct Vector3 {
	float x, y, z;
	Vector3() : x(0), y(0), z(0) {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
	Vector3(float value) : x(value), y(value), z(value) {}
	Vector3 operator+(const Vector3& other) const {
		return Vector3(x + other.x, y + other.y, z + other.z);
	}
	Vector3 operator-(const Vector3& other) const {
		return Vector3(x - other.x, y - other.y, z - other.z);
	}
	Vector3 operator*(float scalar) const {
		return Vector3(x * scalar, y * scalar, z * scalar);
	}
	Vector3 operator/(float scalar) const {
		return Vector3(x / scalar, y / scalar, z / scalar);
	}
	bool operator==(const Vector3& other) const {
		return x == other.x && y == other.y && z == other.z;
	}
	float Length() const {
		return sqrt(x * x + y * y + z * z);
	}
	float Dot(const Vector3& other) const {
		return x * other.x + y * other.y + z * other.z;
	}
	Vector3 Normalize() const {
		float length = Length();
		if (length == 0) return Vector3(0, 0, 0);
		return Vector3(x / length, y / length, z / length);
	}
	static Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
		return a + (b - a) * t;
	}
	static Vector3 Cross(const Vector3& a, const Vector3& b) {
		return Vector3(
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
		);
	}
};

struct Vector4 {
	float x, y, z, w;
	Vector4() : x(0), y(0), z(0), w(0) {}
	Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	Vector4(float value) : x(value), y(value), z(value), w(value) {}
	Vector4 operator+(const Vector4& other) const {
		return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
	}
	Vector4 operator-(const Vector4& other) const {
		return Vector4(x - other.x, y - other.y, z - other.z, w - other.w);
	}
	Vector4 operator*(float scalar) const {
		return Vector4(x * scalar, y * scalar, z * scalar, w * scalar);
	}
	Vector4 operator/(float scalar) const {
		return Vector4(x / scalar, y / scalar, z / scalar, w / scalar);
	}
	bool operator==(const Vector4& other) const {
		return x == other.x && y == other.y && z == other.z && w == other.w;
	}
	float Length() const {
		return sqrt(x * x + y * y + z * z + w * w);
	}
	float Dot(const Vector4& other) const {
		return x * other.x + y * other.y + z * other.z + w * other.w;
	}
	Vector4 Normalize() const {
		float length = Length();
		if (length == 0) return Vector4(0, 0, 0, 0);
		return Vector4(x / length, y / length, z / length, w / length);
	}
	static Vector4 Lerp(const Vector4& a, const Vector4& b, float t) {
		return a + (b - a) * t;
	}
};