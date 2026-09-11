#pragma once
#include <vector>
#include <string>
#include "DXC.h"

namespace GameEngine {

	struct LibraryResult {
		Microsoft::WRL::ComPtr<IDxcBlob> blob;
		std::vector<std::wstring> exportNames;
	};

	class RayLibShaderCompiler {
	public:

		void Initialize(DXC* dxc);

		// シェーダーをコンパイルする
		LibraryResult CompileShader(const std::wstring& hlslPath);

	private:
		DXC* dxc_ = nullptr;

		// csoファイルのディレクトリパス
		const std::wstring csoDirectory_ = L"Resources/Shaders/Compiled/Raytracing/";

		// hlsliの更新チェックに使うシェーダーのルートディレクトリ
		const std::wstring shaderRootDirectory_ = L"Resources/Shaders/";

		const std::wstring kLibProfile_ = L"lib_6_6";

	private:

		// コンパイル済みBlobからエクスポート名を取得
		std::vector<std::wstring> ReflectExportNames(IDxcBlob* blob);

		/// <summary>
		/// HLSLのパスからCSOのパスを生成
		/// </summary>
		std::wstring GetCsoPath(const std::wstring& hlslPath);

		/// <summary>
		/// CSOファイルを読み込む
		/// </summary>
		Microsoft::WRL::ComPtr<IDxcBlob> LoadCsoFile(const std::wstring& csoPath);

		/// <summary>
		/// CSOファイルに保存
		/// </summary>
		void SaveCsoFile(const std::wstring& csoPath, IDxcBlob* blob);

		/// <summary>
		/// HLSL(またはincludeしているhlsli)がCSOより新しいかチェック
		/// </summary>
		bool IsHlslNewer(const std::wstring& hlslPath, const std::wstring& csoPath);

		/// <summary>
		/// HLSLをコンパイルしてCSOとして保存
		/// </summary>
		Microsoft::WRL::ComPtr<IDxcBlob> CompileAndSave(const std::wstring& hlslPath);
	};
}
