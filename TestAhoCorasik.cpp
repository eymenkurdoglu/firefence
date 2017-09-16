#include <stdio.h>
#include "SuffixTrie.h"

int main(int argc, char* argv[])
{
	CSuffixTrie aTree;
	aTree.AddString("barak");
	aTree.AddString("arakoo");
	aTree.AddString("barakoo");
	aTree.AddString("barako565");
	aTree.BuildTreeIndex();
	CSuffixTrie::_DataFound aData;
	CSuffixTrie::DataFoundVector aDataFound;
	aDataFound=aTree.SearchAhoCorasikMultiple("1236h6h6barakoo6arakoo123");

	for (int iCount=0; iCount<aDataFound.size(); ++iCount)
		printf("%s %i\n",aDataFound[iCount].sDataFound.c_str(),aDataFound[iCount].iFoundPosition);

return 0;
}
