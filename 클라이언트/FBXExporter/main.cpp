#include "FBXExporter.h"

int main()
{
	FBXExporter fbxExporter{};
	fbxExporter.Process("target/target.fbx");
}