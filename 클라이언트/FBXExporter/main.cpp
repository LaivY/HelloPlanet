﻿#include "FBXExporter.h"

int main()
{
	FBXExporter fbxExporter{};
	fbxExporter.Process("target/right.fbx");
}