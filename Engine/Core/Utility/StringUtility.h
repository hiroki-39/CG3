#pragma once
#include <string>

//�����R�[�h���[�e�B���e�B
namespace StringUtility
{
	//string��wstring�ɕϊ�
	std::wstring ConvertString(const std::string& str);

	//wstring��string�ɕϊ�
	std::string ConvertString(const std::wstring& str);
}