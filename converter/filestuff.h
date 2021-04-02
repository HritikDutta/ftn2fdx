#pragma once

#include "containers/string.h"

String load_file(const String filepath);
int write_file(const String filepath, String contents);