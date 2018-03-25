#include <stdio.h>
#include <iostream>
#include <string>
#include <ResManager/plResManager.h>
#include <Debug/hsExceptions.hpp>
#include <Stream/hsStdioStream.h>
#include <Debug/plDebug.h>
#include <string_theory/stdio>
#include <cstring>
#include <time.h>

int main(int argc, char** argv){
	std::cout << "Shorah !" << std::endl;
	if(argc < 2){
		return 1;
	}
	const std::string path(argv[1]);
	plResManager rm;
	plPageInfo* page;
	try {
		page = rm.ReadPage(path);
	} catch (hsException& e) {
		ST::printf(stderr, "{}:{}: {}\n", e.File(), e.Line(), e.what());
		return 1;
	} catch (std::exception& e) {
		ST::printf(stderr, "PrcExtract Exception: {}\n", e.what());
		return 1;
	} catch (...) {
		fputs("Undefined error!\n", stderr);
		return 1;
	}
	ST::string outDir = page->getAge();
	std::cout << outDir.to_std_string() << std::endl;
	return 0;
}
