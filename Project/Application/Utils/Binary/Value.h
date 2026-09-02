#pragma once
#include <cstdint>
#include <string>

struct DirectionalLight;

enum class TypeID : uint8_t {
	kUnknown = 0x00,	//不明な型

	//もともとある型
	Int = 0x01,
	Float,
	Bool,
	String,
	Double,
	uint8_t,
	uint16_t,
	uint32_t,
	uint64_t,
	Char,

	//VectorとかMatrixとか
	Vector2,
	Vector3,
	Vector4,
	Matrix2x2,
	Matrix3x3,
	Matrix4x4,
	Quaternion,

	//構造体とか
	Custom = 0x80,
	Transform,
	QTransform,
};

struct Vector2;
struct Vector3;
struct Vector4;
struct Matrix2x2;
struct Matrix3x3;
struct Matrix4x4;
struct Quaternion;
struct Transform;
struct QuaternionTransform;

template<typename T>
struct TypeIDResolver {
	static constexpr TypeID id = TypeID::kUnknown;
};

template<> struct TypeIDResolver<int> { static constexpr TypeID id = TypeID::Int; };
template<> struct TypeIDResolver<float> { static constexpr TypeID id = TypeID::Float; };
template<> struct TypeIDResolver<bool> { static constexpr TypeID id = TypeID::Bool; };
template<> struct TypeIDResolver<std::string> { static constexpr TypeID id = TypeID::String; };
template<> struct TypeIDResolver<double> { static constexpr TypeID id = TypeID::Double; };
template<> struct TypeIDResolver<uint8_t> { static constexpr TypeID id = TypeID::uint8_t; };
template<> struct TypeIDResolver<uint16_t> { static constexpr TypeID id = TypeID::uint16_t; };
template<> struct TypeIDResolver<uint32_t> { static constexpr TypeID id = TypeID::uint32_t; };
template<> struct TypeIDResolver<uint64_t> { static constexpr TypeID id = TypeID::uint64_t; };
template<> struct TypeIDResolver<char> { static constexpr TypeID id = TypeID::Char; };
template<> struct TypeIDResolver<Vector2> { static constexpr TypeID id = TypeID::Vector2; };
template<> struct TypeIDResolver<Vector3> { static constexpr TypeID id = TypeID::Vector3; };
template<> struct TypeIDResolver<Vector4> { static constexpr TypeID id = TypeID::Vector4; };
template<> struct TypeIDResolver<Matrix2x2> { static constexpr TypeID id = TypeID::Matrix2x2; };
template<> struct TypeIDResolver<Matrix3x3> { static constexpr TypeID id = TypeID::Matrix3x3; };
template<> struct TypeIDResolver<Matrix4x4> { static constexpr TypeID id = TypeID::Matrix4x4; };
template<> struct TypeIDResolver<Quaternion> { static constexpr TypeID id = TypeID::Quaternion; };
template<> struct TypeIDResolver<Transform> { static constexpr TypeID id = TypeID::Transform; };
template<> struct TypeIDResolver<QuaternionTransform> { static constexpr TypeID id = TypeID::QTransform; };
