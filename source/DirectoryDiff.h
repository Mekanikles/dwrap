#pragma once

#include "FileUtils.h"

struct DiffEntry
{
	const FileInfo* leftFile;
	const FileInfo* rightFile;
	bool differs;
};

enum DiffType
{
	k2Way,
	k3Way
};

struct DirectoryDiffState
{
	std::vector<FileInfo> leftFiles;
	std::vector<FileInfo> rightFiles;
	std::vector<DiffEntry> sortedEntries;
	DiffType diffType;
	bool hasMergeOutput;
};

void GenerateDirectoryDiffState(const PathSet& paths, not_null<DirectoryDiffState> outDirDiffState)
{
	DiffType diffType = GetPath(paths, kBase) != nullptr ? k3Way : k2Way;

	std::cout << "Diffing as " << ((diffType == k2Way) ? "2way" : "3way") << "\n";

	if (diffType == k2Way)
	{
		const std::string* leftPath = GetPath(paths, kLeft);
		const std::string* rightPath = GetPath(paths, kRight);

		assert(leftPath != nullptr && rightPath != nullptr);

		auto& leftFiles = outDirDiffState->leftFiles;
		leftFiles.clear();
		auto& rightFiles = outDirDiffState->rightFiles;
		rightFiles.clear();
		ListFilesInDirRecursively(*leftPath, "", 0, &leftFiles);
		ListFilesInDirRecursively(*rightPath, "", 0, &rightFiles);
		SortFileList(&leftFiles);
		SortFileList(&rightFiles);

		printf("Left directory:\n");
		for (const auto& f : leftFiles)
			printf("    '%s' (%i)\n", f.relativePath.c_str(), DirLevel(f));

		printf("Right directory:\n");
		for (const auto& f : rightFiles)
			printf("    '%s' (%i)\n", f.relativePath.c_str(), DirLevel(f));	


		outDirDiffState->diffType = k2Way;
		auto& entries = outDirDiffState->sortedEntries;
		entries.clear();

		int leftStepper = 0;
		const int leftCount = leftFiles.size();
		int rightStepper = 0;
		const int rightCount = rightFiles.size();
		while (leftStepper < leftCount && rightStepper < rightCount)
		{
			const FileInfo& leftFile = leftFiles[leftStepper];
			const FileInfo& rightFile = rightFiles[rightStepper];

			printf("Comparing %s, %s:\n", leftFile.relativePath.c_str(), rightFile.relativePath.c_str());

			if (SameRelativeFile(leftFile, rightFile))
			{
				bool differs;
				if (IsDir(leftFile) && IsDir(rightFile))
				{
					printf("    Same directory.\n");
					differs = false;
				}
				else
				{
					printf("    Same file, checking if equal.\n");
					differs = !FileEquals(leftFile, rightFile);
					if (differs)
						printf("File differs.\n");
					else
						printf("Files identical.\n");
				}

				entries.push_back(DiffEntry { &leftFile, &rightFile, differs});

				++leftStepper;
				++rightStepper;
			}
			else
			{
				if (FileInfoSortFunc(leftFile, rightFile))
				{
					printf("    Sole left file found.\n");
					entries.push_back(DiffEntry { &leftFile, nullptr, true });
					++leftStepper;
				}
				else
				{
					printf("    Sole right file found.\n");
					entries.push_back(DiffEntry { nullptr, &rightFile, true});
					++rightStepper;
				}
			}
		}

		// Fill in rest of left files if any
		while (leftStepper < leftCount)
			entries.push_back(DiffEntry { &leftFiles[leftStepper++], nullptr });


		// Fill in rest of right files if any
		while (rightStepper < rightCount)
			entries.push_back(DiffEntry { nullptr, &rightFiles[rightStepper++] });
	}
	else
	{

	}
}



















