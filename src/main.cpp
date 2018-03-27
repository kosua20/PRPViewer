#include <stdio.h>
#include <iostream>
#include <string>
#include <ResManager/plResManager.h>
#include <Debug/hsExceptions.hpp>
#include <Stream/hsStdioStream.h>
#include <Stream/hsStream.h>

#include <Stream/plEncryptedStream.h>
#include <Debug/plDebug.h>
#include <string_theory/stdio>
#include <cstring>
#include <time.h>

std::vector<ST::string> split(const ST::string & str, char split_char, size_t max_splits)
{
	ST_ASSERT(split_char && static_cast<unsigned int>(split_char) < 0x80,
			  "Split character should be in range '\\x01'-'\\x7f'");
	
	std::vector<ST::string> result;
	
	const char *next = str.c_str();
	const char *end = next + str.size();
	while (max_splits) {
		const char *sp = strchr(next, split_char);
		if (!sp)
			break;
		
		result.push_back(ST::string(next, sp - next, ST::assume_valid));
		next = sp + 1;
		--max_splits;
	}
	
	result.push_back(ST::string(next, end - next, ST::assume_valid));
	return result;
}

int main(int argc, char** argv){
	std::cout << "Shorah !" << std::endl;
	if(argc < 2){
		return 1;
	}
	const std::string path(argv[1]);
	plResManager rm;
	plAgeInfo* age;
	try {
		age = new plAgeInfo();
		age->readFromFile(path);
		
		

		
	} catch (hsException& e) {
		ST::printf(stderr, "{}:{}: {}\n", e.File(), e.Line(), e.what());
		return 1;
	} catch (std::exception& e) {
		ST::printf(stderr, "Prc Extract Exception: {}\n", e.what());
		return 1;
	} catch (...) {
		fputs("Undefined error!\n", stderr);
		return 1;
	}
	
	ST::string outDir = age->getAgeName();
	std::cout << outDir.to_std_string() << ":" << age->getNumPages() << " pages." << std::endl;
	return 0;
}
