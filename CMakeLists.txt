cmake_minimum_required(VERSION 3.7)
project(OOP_lab3)

set(SOURCE_FILES
	src/main.cpp
	src/wav_core.cpp
	src/wav_core.h
	src/wav_header.h
	src/wav.h
	src/wav.cpp
	src/WavExceptions.h
	)


add_executable(OOP_lab3 ${SOURCE_FILES})