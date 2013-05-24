// NslTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>

#include "gtest/gtest.h"
#include "include/nsl.h"

int main(int argc, char** argv) 
{ 
    testing::InitGoogleTest(&argc, argv); 

	// run unit tests
	testing::GTEST_FLAG(filter) = "*Unit*";

    RUN_ALL_TESTS(); 

	std::getchar(); // keep console window open until Return keystroke
	
}

