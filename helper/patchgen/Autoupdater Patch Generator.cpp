#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <windows.h>
#include "md5.hpp"
using namespace std;


// Файл
struct SFile
{
	string       Path;
	unsigned int Size;
	char         Hash[33];
};
typedef vector<SFile> FilesList;

MD5 Hasher;
string ExeName;

// Сформировать список файлов
void GetFileList(const string& path, FilesList& files, unsigned int& total_size);
// Получить название .exe файла
string ExtractFileName(const string& str);
// Проверить что директория существует
bool dirExists(const std::string& dirName_in);
// Сгенерировать список файлов
bool BuildFileList(const std::string& Path, const FilesList& Files);
// Получить размер файла
int GetFileSize(const std::string Path);

// Точка входа
int main(int argc, char *argv[])
{
	/*
	setlocale(LC_ALL, "Russian");
	cout << "Генератор списка файлов для сервера автообновлений." << endl;
	cout << "Подождите пока будет составлен список файлов эталонного клиента." << endl;
	cout << "Это может занять продолжительное время..." << endl << endl;
	*/
	
	cout << "The file list generator for the auto-update server." << endl;
	cout << "The list of patches is currently being generated." << endl;
	cout << "Please allow a few minutes for this process to complete ..." << endl << endl;
	

	// Создаем папку для программы автообновлений
	if (!dirExists("updater"))
	{
		if (!CreateDirectoryA("updater", NULL)) {
			//cout << "Не удалось создать директорию 'updater'!" << endl;
			cout << "Could not create the 'updater' directory!" << endl;
			system("PAUSE");

			return 1;
		}
	}

	// Засекаем время
	clock_t t_begin = clock();
	clock_t t_end = 0;

	// Получаем название исполняемого файла программы
	ExeName = ExtractFileName(argv[0]);

	// Сканируем клиент
	FilesList files;
	unsigned int total_size = 0;
	GetFileList("", files, total_size);

	// Создаем файл с эталонным списком файлов
	if (!BuildFileList("updater\\vercomp.dat", files))
	{
		// cout << "Не удалось создать файл с эталонным списком файлов!" << std::endl;
		system("PAUSE");
		return 1;
	}
	
	// Создаем файл с хэшем эталонного списка файлов
	ofstream ver("updater\\ver.dat", ofstream::binary);
	if (ver.fail()) {
		//cout << "Не удалось открыть (создать) файл ver.dat!" << endl;
		cout << "Could not open (create) ver.dat file!" << endl;
		system("PAUSE");

		return 1;
	}

	// Пишем хэш
	char *vercomp_hash = Hasher.digestFile("updater\\vercomp.dat");
	ver.write(vercomp_hash, 32);
	ver.close();

	// Открываем файл с проверками
	std::ifstream check_list("check.txt");
	if (check_list.is_open())
	{
		std::string FilePath;
		FilesList CheckFiles;

		// Читаем список файлов
		while (getline(check_list, FilePath))
		{
			SFile File;

			File.Size = GetFileSize(FilePath);
			if (File.Size == 0)
			{
				//std::cout << "Ошибка: файл " << FilePath << " не найден!" << std::endl;
				std::cout << "Error: File " << FilePath << " not found!" << std::endl;
				continue;
			}

			File.Path = FilePath;
			strcpy(File.Hash, Hasher.digestFile(File.Path.c_str()));
			CheckFiles.push_back(File);
		}

		// Создаем файл с проверяемыми файлами
		if (!BuildFileList("updater\\check.dat", CheckFiles))
		{
			//std::cout << "Не удалось создать файл с эталонным списком файлов!" << std::endl;
			system("PAUSE");
			return 1;
		}
	}

	t_end = clock();
	int time_diff = (t_end - t_begin) / CLOCKS_PER_SEC;
	float mbytes = static_cast<float>(total_size) / 1024.0f / 1024.0f;

	
	/*
	cout << "Готово!" << endl;
	cout << "Всего файлов: " << n << endl;
	cout << "Размер клиента: " << mbytes << " Мб" << endl;
	cout << "Затраченное время: " << time_diff << " секунд." << endl;
	*/
	
	cout << "Done!" << endl;
	cout << "Total files: " << files.size() << endl;
	cout << "Client size: " << mbytes << " Mb" << endl;
	cout << "Elapsed time: " << time_diff << " sec." << endl;


	system("PAUSE");
    return 0;
}

// Сформировать список файлов
void GetFileList(const string& path, FilesList& files, unsigned int& total_size)
{
	WIN32_FIND_DATAA FindFileData;
	HANDLE hFind = FindFirstFileA((path + "*.*").c_str(), &FindFileData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			string file_name = FindFileData.cFileName;
			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (file_name != "." && file_name != "..") {
					GetFileList((path + file_name + "\\").c_str(), files, total_size);
				}
			}
			else
			{
				if (file_name == "check.dat" || file_name == "check.txt" || file_name == "ver.dat" || file_name == "vercomp.dat" || file_name == ExeName)
					continue;

				SFile File;
				File.Path = path + file_name;
				File.Size = (FindFileData.nFileSizeHigh * (MAXDWORD + 1)) + FindFileData.nFileSizeLow;
				strcpy(File.Hash, Hasher.digestFile(File.Path.c_str()));
				total_size += File.Size;

				files.push_back(File);
			}
		}
		while (FindNextFileA(hFind, &FindFileData));
	}

	FindClose(hFind);
}

// Получить название .exe файла
string ExtractFileName(const string& str)
{
	size_t index = 0;
	string path = str;

	replace(path.begin(), path.end(), '/', '\\');
	index = path.find_last_of('\\');

	return path.substr(index + 1);
}


bool dirExists(const std::string& dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}


// Сгенерировать список файлов
bool BuildFileList(const std::string& Path, const FilesList& Files)
{
	char *buffer = new char[8 * 1024 * 1024];
	unsigned int pos, n, total_size;
	pos = total_size = 0;
	n = Files.size();

	memcpy(buffer, (char *)&n, sizeof(n));
	pos += sizeof(n);
	for (unsigned int i = 0; i < n; i++)
	{
		unsigned short int len = static_cast<unsigned short int>(Files[i].Path.length());
		memcpy(buffer + pos, (char *)&len, sizeof(len));
		pos += sizeof(len);

		memcpy(buffer + pos, Files[i].Path.c_str(), len);
		pos += len;

		memcpy(buffer + pos, (char *)&Files[i].Size, sizeof(Files[i].Size));
		pos += sizeof(Files[i].Size);

		memcpy(buffer + pos, Files[i].Hash, sizeof(Files[i].Hash));
		pos += sizeof(Files[i].Hash);

		total_size += Files[i].Size;
	}

	ofstream vercomp(Path, ofstream::binary);
	if (vercomp.fail()) {
		delete[] buffer;
		cout << "Не удалось открыть (создать) файл " << ExtractFileName(Path) << "!" << endl;
		//cout << "Could not open (create) vercomp.dat file!" << endl;
		system("PAUSE");

		return false;
	}

	vercomp.write(buffer, pos);
	vercomp.close();
	delete[] buffer;

	return true;
}

// Получить размер файла
int GetFileSize(const std::string Path)
{
	int size = 0;
	std::ifstream file(Path);
	if (file.is_open())
	{
		file.seekg(0, file.end);
		size = static_cast<int>(file.tellg());
		file.close();
	}

	return size;
}