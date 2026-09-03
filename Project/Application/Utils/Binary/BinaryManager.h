#pragma once
#include <string>
#include <cstring>
#include "Value.h"

class BinaryManager {
public:

	bool Boot(const std::string& fileName);
	void BootRawData(const std::string& rawData);

	template<typename T>
	void Register(const T* data);
	void Write(const std::string& fileName);

	//Registerした順番で値を吐き出す。一回しか吐き出さない。
	template<typename T>
	T Reverse();
	void Back() { readIndex_ = prevReadIndex_; }

	bool IsEmpty() const { return inputBuffer_.empty(); }

	std::string GetOutputRawData() const { return outputBuffer_; }
	std::string GetInputRawData() const { return inputBuffer_; }

private:

	std::string inputBuffer_;
	std::string outputBuffer_;

	//TypeIDと混在しないように、バージョンは0xf0以上にする
	static constexpr uint8_t version = 0xf0;
	uint8_t version_ = version;

	uint32_t readIndex_ = 0;
	uint32_t prevReadIndex_ = 0;

	static inline const std::string basePath = "Assets/Binary/";

	static constexpr uint32_t idSize = uint32_t(sizeof(TypeID));
	static constexpr uint32_t sizeSize = uint32_t(sizeof(uint32_t));
	static constexpr uint32_t headerSize = idSize + sizeSize;
};

template<typename T>
void BinaryManager::Register(const T* data) {
	constexpr TypeID currentID = TypeIDResolver<T>::id;
	uint32_t size = uint32_t(sizeof(T));

	// 未対応の型の場合は登録しない
	if (currentID == TypeID::kUnknown) {
		return;
	}

	/* ID->Size->値 */
	outputBuffer_.append(reinterpret_cast<const char*>(&currentID), sizeof(TypeID));
	outputBuffer_.append(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
	outputBuffer_.append(reinterpret_cast<const char*>(data), size);
}

template<typename T>
T BinaryManager::Reverse() {
	T value{};
	prevReadIndex_ = readIndex_;

	if (inputBuffer_.size() < headerSize) {
		return value;
	}

	TypeID id;
	uint32_t size;

	std::memcpy(&id, inputBuffer_.data() + readIndex_, idSize);
	std::memcpy(&size, inputBuffer_.data() + readIndex_ + idSize, sizeSize);

	if (id != TypeIDResolver<T>::id) {
		return value;
	}

	if (inputBuffer_.size() < headerSize + size) {
		return value;
	}

	std::memcpy(&value, inputBuffer_.data() + readIndex_ + headerSize, sizeof(T));
	readIndex_ += headerSize + size;

	return value;
}

template<>
inline void BinaryManager::Register<std::string>(const std::string* data) {
	constexpr TypeID id = TypeIDResolver<std::string>::id;
	uint32_t size = uint32_t(data->size());

	outputBuffer_.append(reinterpret_cast<const char*>(&id), sizeof(id));
	outputBuffer_.append(reinterpret_cast<const char*>(&size), sizeof(size));
	outputBuffer_.append(data->data(), size);
}

template<>
inline std::string BinaryManager::Reverse<std::string>() {
	std::string value;
	prevReadIndex_ = readIndex_;

	if (inputBuffer_.size() < headerSize) {
		return value;
	}

	TypeID id;
	uint32_t size;
	std::memcpy(&id, inputBuffer_.data() + readIndex_, idSize);
	std::memcpy(&size, inputBuffer_.data() + readIndex_ + idSize, sizeSize);

	if (id != TypeIDResolver<std::string>::id) {
		return value;
	}

	if (inputBuffer_.size() < headerSize + size) {
		return value;
	}

	value.resize(size);
	std::memcpy(value.data(), inputBuffer_.data() + readIndex_ + headerSize, size);
	readIndex_ += headerSize + size;

	return value;
}
