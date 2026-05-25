#pragma once

#include <string>
#include <ShObjIdl.h> // IFileOpenDialog

std::string OpenFileDialog(const COMDLG_FILTERSPEC* pFilterSpecs = nullptr, UINT filterCount = 0);