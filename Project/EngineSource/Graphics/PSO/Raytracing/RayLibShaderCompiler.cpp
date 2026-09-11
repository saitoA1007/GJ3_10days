#include "RayLibShaderCompiler.h"
#include <cassert>
#include <fstream>
#include <filesystem>
#include <d3dcompiler.h>
#include <d3d12shader.h>

namespace fs = std::filesystem;
using namespace GameEngine;

void RayLibShaderCompiler::Initialize(DXC* dxc) {
	dxc_ = dxc;

	// CSOディレクトリが存在しない場合は作成
	fs::path csoDir(csoDirectory_);
	if (!fs::exists(csoDir)) {
		fs::create_directories(csoDir);
	}
}

LibraryResult RayLibShaderCompiler::CompileShader(const std::wstring& hlslPath) {

	LibraryResult result;

	std::wstring csoPath = GetCsoPath(hlslPath);

#ifdef _DEBUG
	// Debug版ではHLSLが更新されていればコンパイル、なければCSOを読み込む
	if (!fs::exists(csoPath) || IsHlslNewer(hlslPath, csoPath)) {
		// コンパイルして保存
		result.blob = CompileAndSave(hlslPath);
	} else {
		// 既存のCSOを読み込み
		result.blob = LoadCsoFile(csoPath);

		// 読み込みに失敗した場合はコンパイルし直す
		if (result.blob == nullptr) {
			result.blob = CompileAndSave(hlslPath);
		}
	}
#else
	// Release版ではCSOファイルを読み込む
	result.blob = LoadCsoFile(csoPath);
	assert(result.blob != nullptr && "CSO file not found in Release build");
#endif

	// CSOから読み込んだBlobでもDXILコンテナなのでリフレクションできる
	result.exportNames = ReflectExportNames(result.blob.Get());

	return result;
}

std::wstring RayLibShaderCompiler::GetCsoPath(const std::wstring& hlslPath) {
	// パスからファイル名を取得
	fs::path path(hlslPath);
	std::wstring filename = path.filename().wstring();

	// 拡張子を.csoに変更
	size_t dotPos = filename.find_last_of(L'.');
	if (dotPos != std::wstring::npos) {
		filename = filename.substr(0, dotPos) + L".cso";
	} else {
		filename += L".cso";
	}

	// CSOディレクトリのパスと結合
	return csoDirectory_ + filename;
}

Microsoft::WRL::ComPtr<IDxcBlob> RayLibShaderCompiler::LoadCsoFile(const std::wstring& csoPath) {
	// ファイルを開く
	std::ifstream file(csoPath, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		return nullptr;
	}

	// ファイルサイズを取得
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	// データを読み込む
	std::vector<char> buffer(size);
	if (!file.read(buffer.data(), size)) {
		return nullptr;
	}
	file.close();

	// IDxcBlobを作成
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> blob;
	HRESULT hr = dxc_->dxcUtils_->CreateBlob(
		buffer.data(),
		static_cast<UINT32>(size),
		CP_ACP,
		&blob
	);

	if (FAILED(hr)) {
		return nullptr;
	}

	return blob;
}

void RayLibShaderCompiler::SaveCsoFile(const std::wstring& csoPath, IDxcBlob* blob) {
	// ファイルに書き込む
	std::ofstream file(csoPath, std::ios::binary);
	if (!file.is_open()) {
		assert(false && "Failed to open CSO file for writing");
		return;
	}
	file.write(static_cast<const char*>(blob->GetBufferPointer()), blob->GetBufferSize());
	file.close();
}

bool RayLibShaderCompiler::IsHlslNewer(const std::wstring& hlslPath, const std::wstring& csoPath) {
	fs::path hlsl(hlslPath);
	fs::path cso(csoPath);

	if (!fs::exists(hlsl) || !fs::exists(cso)) {
		return true;
	}

	// 更新日時を比較
	auto csoTime = fs::last_write_time(cso);
	if (fs::last_write_time(hlsl) > csoTime) {
		return true;
	}

	// includeしているhlsliが更新されている場合も再コンパイルする
	fs::path root(shaderRootDirectory_);
	if (fs::exists(root)) {
		for (const auto& entry : fs::recursive_directory_iterator(root)) {
			if (!entry.is_regular_file()) {
				continue;
			}
			if (entry.path().extension() != L".hlsli") {
				continue;
			}
			if (fs::last_write_time(entry) > csoTime) {
				return true;
			}
		}
	}

	return false;
}

Microsoft::WRL::ComPtr<IDxcBlob> RayLibShaderCompiler::CompileAndSave(const std::wstring& hlslPath) {
	// HLSLをコンパイル
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = dxc_->CompileShader(hlslPath, kLibProfile_.c_str(), L"");

	assert(shaderBlob != nullptr && "Shader compilation failed");

	// CSOファイルとして保存
	std::wstring csoPath = GetCsoPath(hlslPath);
	SaveCsoFile(csoPath, shaderBlob.Get());
	return shaderBlob;
}

std::vector<std::wstring> RayLibShaderCompiler::ReflectExportNames(IDxcBlob* blob) {
    std::vector<std::wstring> names;

    // コンテナリフレクションを作成
    Microsoft::WRL::ComPtr<IDxcContainerReflection> containerReflection;
    HRESULT hr = DxcCreateInstance(
        CLSID_DxcContainerReflection,
        IID_PPV_ARGS(&containerReflection)
    );
    assert(SUCCEEDED(hr));

    hr = containerReflection->Load(blob);
    assert(SUCCEEDED(hr));

    // DXILパートを探す
    UINT32 partIndex = 0;
    hr = containerReflection->FindFirstPartKind(
        DXC_PART_DXIL, // 0x4C495844
        &partIndex
    );
    assert(SUCCEEDED(hr));

    // ID3D12LibraryReflectionを取得
    Microsoft::WRL::ComPtr<ID3D12LibraryReflection> libraryReflection;
    hr = containerReflection->GetPartReflection(
        partIndex,
        IID_PPV_ARGS(&libraryReflection)
    );
    assert(SUCCEEDED(hr));

    // エクスポート関数名を列挙
    D3D12_LIBRARY_DESC libDesc{};
    libraryReflection->GetDesc(&libDesc);

    for (UINT i = 0; i < libDesc.FunctionCount; ++i) {
        auto* func = libraryReflection->GetFunctionByIndex(i);
        D3D12_FUNCTION_DESC funcDesc{};
        func->GetDesc(&funcDesc);

        std::string rawName(funcDesc.Name);
        // 先頭の\x1?を取り除く
        if (rawName.size() > 2 && rawName[0] == '\x1' && rawName[1] == '?') {
            rawName = rawName.substr(2);
            // @@以降のシグネチャ部分を除去
            auto at = rawName.find("@@");
            if (at != std::string::npos) {
                rawName = rawName.substr(0, at);
            }
        }
        names.push_back(std::wstring(rawName.begin(), rawName.end()));
    }

    return names;
}
