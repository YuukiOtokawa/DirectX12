#include "FilePicker.h"

std::string OpenFileDialog(const COMDLG_FILTERSPEC* pFilterSpecs, UINT filterCount) {
	IFileOpenDialog* pDialog = nullptr;
	auto rs = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDialog));
	if (FAILED(rs)) {
		pDialog->Release();
		return "";
	}

	if (pFilterSpecs != nullptr && filterCount > 0) {
		pDialog->SetFileTypes(filterCount, pFilterSpecs);
	}
	else {
		COMDLG_FILTERSPEC defaultFilter = { L"All Files", L"*.*" };
		pDialog->SetFileTypes(1, &defaultFilter);
	}

	rs = pDialog->Show(nullptr);

	if (FAILED(rs)) {
		pDialog->Release();
		return "";
	}

	IShellItem* pItem = nullptr;
	rs = pDialog->GetResult(&pItem);
	if (FAILED(rs)) {
		pDialog->Release();
		return "";
	}

	PWSTR pszFilePath = nullptr;
	rs = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
	pDialog->Release();
	pItem->Release();

	int len = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, nullptr, 0, nullptr, nullptr);
	std::string filePath(len - 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &filePath[0], len, nullptr, nullptr);
	CoTaskMemFree(pszFilePath);

	return filePath;
}
