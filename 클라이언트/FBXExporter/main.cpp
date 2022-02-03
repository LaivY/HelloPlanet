#include "FBXExporter.h"

int main()
{
	FBXExporter fbxExporter{};
	fbxExporter.Process("target/player.fbx", "result/");
}