#include "BinaryManager.h"
#include <filesystem>
#include <fstream>
#include <ostream>

namespace fs = std::filesystem;

void BinaryManager::Write(const std::string& fileName) {
	std::ofstream file(basePath + fileName, std::ios::binary);

	if (!file) {
		fs::create_directories(basePath); // ディレクトリが存在しない場合は作成
		file.open(basePath + fileName, std::ios::binary); // 再度ファイルを開く

		if (!file) {
			// 作成に失敗している状態。
			throw std::runtime_error("Failed to open file for writing: " + basePath + fileName);
		}
	}

	file.write(reinterpret_cast<const char*>(&version_), sizeof(version_)); // バージョンを書き込む
	file.write(outputBuffer_.data(), outputBuffer_.size());

	file.close();

	// 書き込み後はバッファをクリア
	outputBuffer_.clear();
}

bool BinaryManager::Boot(const std::string& fileName) {
	const std::string path = basePath + fileName;
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) {
		return false;
	}

	const auto fileSize = fs::file_size(path);
	inputBuffer_.resize(static_cast<size_t>(fileSize));
	file.read(inputBuffer_.data(), fileSize);

	uint8_t fileVersion;
	std::memcpy(&fileVersion, inputBuffer_.data(), sizeof(fileVersion));

	//Versionが違う場合は読み込まない
	if (fileVersion != version) {
		inputBuffer_.clear();
		return false;
	}

	inputBuffer_.erase(0, sizeof(fileVersion)); // バージョンを読み取った後、バッファから削除 
	readIndex_ = 0; // 読み取りインデックスをリセット

	return true;
}

void BinaryManager::BootRawData(const std::string& rawData) {
	inputBuffer_ = rawData;
	readIndex_ = 0; // 読み取りインデックスをリセット
}
