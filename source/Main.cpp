#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>
#include <thread>

#include "Common.h"
#include "FileUtils.h"
#include "DirectoryDiff.h"
#include "GUI.h"

struct RunParams
{
	std::string tool;
	std::vector<std::string> toolParams;
	PathSet paths;

	bool noGUI;
	bool allowMultipleDiffs;
};

void ParseDiffToolCommand(const std::string& toolArgs, not_null<RunParams> outRunParams)
{
	std::istringstream buffer(toolArgs);

	buffer >> outRunParams->tool;
	std::string param;
	while (buffer >> param)
	{
		if (param.c_str()[0] == '@')
			outRunParams->toolParams.push_back(param);
		else
			outRunParams->tool += " " + param;
	}
}

bool InitRunParams(const std::vector<std::string>& arguments, not_null<RunParams> outRunParams)
{
	outRunParams->noGUI = false;
	outRunParams->allowMultipleDiffs = false;

	for (const auto& s : arguments)
		printf("%s\n", s.c_str());

	for (int i = 0, argCount = arguments.size(); i < argCount; ++i)
	{
		const std::string& s = arguments[i];
		if (s.c_str()[0] == '-')
		{
			if (s == "--tool")
			{
				if (i >= argCount - 1)
				{
					std::cerr << "Error: param '-tool' found but no tool command supplied.\n";
					return EX_IOERR;
				}

				ParseDiffToolCommand(arguments[++i], outRunParams);
			}
			else if (s == "--noGUI")
			{
				outRunParams->noGUI = true;
			}
			else if (s == "--allowMultipleDiffs")
			{
				outRunParams->allowMultipleDiffs = true;
			}
			else
			{
				std::cerr << "Error: unrecognized parameter '" << s.c_str() << "'.\n";
			}
		}
		else
		{
			outRunParams->paths.push_back(s);
		}
	}

	return true;
}

bool VerifyPathParams(const PathSet& paths, not_null<bool> outAllRegularFiles, not_null<bool> outAllDirectories)
{
	bool allFiles = true;
	bool allDirs = true;
	for (const auto& p : paths)
	{
		bool isFile = false;
		bool isDir = false;
		IsRegularFileOrDirectory(p, &isFile, &isDir);
		allFiles &= isFile;
		allDirs &= isDir;

		if (!isFile && !isDir)
		{
			std::cerr << "Error: Path '" << p << "' is not a file nor a directory.\n";
			return false;
		}
	}

	*outAllRegularFiles = allFiles;
	*outAllDirectories = allDirs;

	return true;
}

int CallDiffTool(const RunParams& runParams, const PathSet& files)
{
	printf("Parsing command with %lu files.\n", files.size());

	std::stringstream stringBuilder;

	stringBuilder << runParams.tool;
	if (runParams.toolParams.size() > 0)
	{
		// TODO: Parse and verify tool params only once
		for (const std::string& param : runParams.toolParams)
		{
			if (param.c_str()[0] == '@')
			{
				if (param.compare(1, 4, "FILE") == 0)
				{
					long index = std::strtol(&param.c_str()[5], nullptr, 10) - 1;
					if (index < 0 || index >= files.size())
					{
						std::cerr << "Error: file index in parameter '" << param << "' out of range.\n";
						return EX_IOERR;
					}

					stringBuilder << " " << files[index];
				}
			}
			else
			{
				stringBuilder << " " << param;
			}

			printf("    Parsed %s to %s\n", param.c_str(), stringBuilder.str().c_str());
		}
	}
	else
	{
		// if we had no command parameters, just append the original files in order
		for (const auto& f : files)
			stringBuilder << " " << f;
	}

	auto str = stringBuilder.str();
	printf("Command Executed: %s\n", str.c_str());


	return system(str.c_str());;
}

struct Worker
{
	std::thread thread;
	std::atomic<bool> done;
};
static std::vector<Worker*> s_workers;

static RunParams s_runParams;

void DoCallDiffTool(const DiffEntry& entry)
{
	PathSet paths;
	if (entry.leftFile)
		paths.push_back(entry.leftFile->absolutePath);
	if (entry.rightFile)
	paths.push_back(entry.rightFile->absolutePath);

	printf("Diffing '%s' and '%s'\n", paths[0].c_str(), paths[1].c_str());

	RunParams& runParams = s_runParams;	
	if (runParams.allowMultipleDiffs)
	{
		Worker* worker = new Worker();
		worker->done = false;
		auto* doneAtomicPtr = &worker->done;
		worker->thread = std::thread([paths, &runParams, doneAtomicPtr]() 
		{
            CallDiffTool(runParams, paths);
            *doneAtomicPtr = true;
        });

		s_workers.push_back(worker);
	}
	else	
		CallDiffTool(runParams, paths);
}

int main(const int argc, const char* argv[])
{
	std::vector<std::string> arguments(argv + 1, argv + argc);

	RunParams& runParams = s_runParams;
	if (!InitRunParams(arguments, &runParams))
		return EX_USAGE;

	if (!runParams.noGUI && runParams.tool.empty())
	{
		std::cerr << "Error: Must supply diff tool for folder compare view. Use '--tool <tool>' to set tool or '--noGUI' to disable folder compare view.\n";
		return EX_USAGE;
	}

	if (!GetPath(runParams.paths, kLeft) || !GetPath(runParams.paths, kRight))
	{
		std::cerr << "Error: You must supply at least two paths to diff.\n";
		return EX_USAGE;
	}

	bool allRegularFiles = true;
	bool allDirectories = false;
	if (!VerifyPathParams(runParams.paths, &allRegularFiles, &allDirectories))
		return EX_IOERR;

	if (!allRegularFiles && !allDirectories)
	{
		std::cerr << "Error: Cannot diff both files and directories at the same time.\n";
		return EX_IOERR;
	}

	int retCode = EX_OK;

	if (allRegularFiles)
	{
		return CallDiffTool(runParams, runParams.paths);
	}
	else
	{
		std::cout << "Diffing directories.\n";
		DirectoryDiffState diffState;
		GenerateDirectoryDiffState(runParams.paths, &diffState);


		printf("Diff result:\n");

		for (const DiffEntry& entry : diffState.sortedEntries)
		{
			assert(entry.leftFile != entry.rightFile);
			if (entry.leftFile == nullptr)
				printf("    [+] '%s'\n", entry.rightFile->relativePath.c_str());
			else if (entry.rightFile == nullptr)
				printf("    [-] '%s'\n", entry.leftFile->relativePath.c_str());	
			else if (entry.differs)
				printf("    [M] '%s'\n", entry.leftFile->relativePath.c_str());	
			else
				printf("    [=] '%s'\n", entry.leftFile->relativePath.c_str());	
		}

		if (runParams.noGUI)
		{
			printf("Diff result:\n");

			if (!runParams.tool.empty())
			{
				printf("Using tool to compare modified files...\n");

				for (const DiffEntry& entry : diffState.sortedEntries)
				{
					if (entry.leftFile != nullptr && entry.rightFile != nullptr)
					{
						if (entry.leftFile->isDir)
							continue;

						DoCallDiffTool(entry);
					}
				}
			}
		}
		else
		{
			GUI::InitWindow(800, 600, diffState, DoCallDiffTool);
			retCode = GUI::Run();
		}
	}

	// Check if any worker is still running
	bool allDone = true;
	for (const Worker* worker : s_workers)
	{
		if (!worker->done)
		{
			allDone = false;
			break;
		}
	}

	if (!allDone)
	{
		printf("Waiting for diff tool to exit...\n");
	}

	// Wait for all workers to quit and clean up
	for (Worker* worker : s_workers)
	{
		worker->thread.join();
		delete worker;
	}
	s_workers.clear();
}


