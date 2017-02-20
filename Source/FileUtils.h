#pragma once
#include <algorithm>
#include <vector>
#include <string>

#include <dirent.h>

enum PathId
{
	kBase,
	kLeft,
	kRight,
	kMerge
};

using PathSet = std::vector<std::string>;

const std::string* GetPath(const PathSet& pathSet, PathId id)
{
	const int size = pathSet.size();
	if (size <= 0)
		return nullptr;

	switch (id)
	{
		case kBase:
		{
			return (size > 2) ? &pathSet[0] : nullptr;
			break;
		}
		case kLeft:
		{
			return (size <= 2) ? &pathSet[0] : ((size > 2) ? &pathSet[1] : nullptr);
			break;
		}
		case kRight:
		{
			return (size <= 2) ? &pathSet[1] : ((size > 2) ? &pathSet[2] : nullptr);
			break;
		}
		case kMerge:
		{
			return (size > 3) ? &pathSet[3] : nullptr;
			break;
		}		
	}

	return nullptr;
}

void IsRegularFileOrDirectory(const std::string& path, not_null<bool> outIsRegFile, not_null<bool> outIsDirectory)
{
    struct stat pathStat;
    stat(path.c_str(), &pathStat);
    *outIsRegFile = S_ISREG(pathStat.st_mode);
	*outIsDirectory = S_ISDIR(pathStat.st_mode);
}


struct FileInfo
{
	std::string relativePath;
	std::string absolutePath;
	std::string name;
	bool isDir;
	int level;
	struct stat status;
};

bool IsDir(const FileInfo& f) { return f.isDir; }
int DirLevel(const FileInfo& f) { return f.level; }
int FileSize(const FileInfo& f) { return (int)f.status.st_size; }

bool ListFilesInDirRecursively(const std::string& baseDir, const std::string& relDir, const int dirLevel, not_null<std::vector<FileInfo>> outFiles)
{
	const std::string dirPath = baseDir + "/" + relDir;

	DIR* dir;
	if ((dir = opendir(dirPath.c_str())) != nullptr) 
	{
		struct dirent* entry;
		while ((entry = readdir(dir)) != nullptr)
		{
			if (strcmp(entry->d_name, ".") == 0)
				continue;
			else if (strcmp(entry->d_name, "..") == 0)
				continue;

		    FileInfo fileInfo;
		    fileInfo.name = entry->d_name;
		    fileInfo.relativePath = relDir.empty() ?  entry->d_name : relDir + "/" + entry->d_name;
		    fileInfo.absolutePath = dirPath + "/" + entry->d_name;
		    struct stat status;
		   	stat(fileInfo.absolutePath.c_str(), &status);
		   	fileInfo.status = status;

		    if (S_ISDIR(status.st_mode))
		    {
		    	fileInfo.isDir = true;
		    	fileInfo.level = dirLevel + 1;
		    	outFiles->push_back(fileInfo);

		    	if (!ListFilesInDirRecursively(baseDir, fileInfo.relativePath, dirLevel + 1, outFiles))
		    	{
		    		closedir(dir);
		    		return false;
		    	}
		    }
		    else if (S_ISREG(status.st_mode))
		    {
		    	fileInfo.isDir = false;
		    	fileInfo.level = dirLevel;
		    	outFiles->push_back(fileInfo);	
		    }
		    else
		    {
				LogLine(kDebug, "Skipping file '%s'", entry->d_name);
		    }
		}
		closedir(dir);
	}
	else
	{
		LogLine(kError, "Could not read directory '%s'", dirPath.c_str());
		return false;
	}

	return true;
}

bool SameRelativeFile(const FileInfo& f1, const FileInfo f2)
{	
	return (IsDir(f1) == IsDir(f2)) && (f1.relativePath == f2.relativePath);
}

bool FileEquals(const FileInfo& f1, const FileInfo& f2)
{
	if (FileSize(f1) != FileSize(f2))
		return false;

	std::ifstream fileStream1;
	std::ifstream fileStream2;
	fileStream1.open(f1.absolutePath, std::ios::binary);
	fileStream2.open(f2.absolutePath, std::ios::binary);

	if (fileStream1.fail() || fileStream2.fail())
		return false; //TODO.

	const int kBufSize = 256;
	char buf1[kBufSize];
	char buf2[kBufSize];
	while(!fileStream1.eof())
	{
		const unsigned int bytesRead1 = (unsigned int)fileStream1.read(buf1, kBufSize).gcount();
		const unsigned int bytesRead2 = (unsigned int)fileStream2.read(buf2, kBufSize).gcount();
		assert(bytesRead1 == bytesRead2);
		if(memcmp(buf1, buf2, bytesRead1) != 0)
		{
			return false;
		}
	}

	return true;
}

bool FileInfoSortFunc(const FileInfo& f1, const FileInfo& f2)
{	
	return f1.relativePath < f2.relativePath;
}

void SortFileList(not_null<std::vector<FileInfo>> files)
{
	std::sort(files->begin(), files->end(), &FileInfoSortFunc);
}





