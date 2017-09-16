#include <map>
#include <string>
#include <vector>
#include <set>

class CSuffixTrie {

public:
	//Our string type
    typedef std::string SearchString;

    static int string_id;
    
    //Data returned from our search
	typedef struct _DataFound {
		int				iFoundPosition;
        int             rule_id;
		SearchString	sDataFound;
	} DataFound;

	//Our vector of data found
	typedef std::vector<DataFound> DataFoundVector;

	//All the strings vector
	typedef std::vector<SearchString> StringsVector;

	//All the strings set
	typedef std::set<SearchString> StringsSet;

public:
	//Get all the strings in the tree
	//Vector format
	StringsVector GetAllStringsVector()const;

	//Set format
	StringsSet GetAllStringsSet()const;

	//Clear the trie
	void Clear();

	//Build the tree index for Aho-Corasick
	//This is done when all the strings has been added
	void BuildTreeIndex();

	//Add a string (will destroy normalization, caller is reponsible for this part)
	void AddString(const SearchString& rString);

	//Get string (is the string there?)
	bool FindString(const SearchString& rString)const;

	//Delete a string (will destroy normalization, caller is reponsible for this part)
	void DeleteString(const SearchString& rString);

	//Do an actual find for the first match
	DataFound SearchAhoCorasik(const SearchString& rString)const;

	//Do an actual find for all the matches
	DataFoundVector SearchAhoCorasikMultiple(const SearchString& rString)const;

	//Assigmnet operator
	CSuffixTrie& operator=(const CSuffixTrie& rTrie);

	//Ctor and Dtor
	CSuffixTrie();
	CSuffixTrie(const CSuffixTrie& rTrie);
	virtual ~CSuffixTrie();
private:
	//Our char search type
//	typedef wchar_t SearchChar; //********************************************************************
    typedef char SearchChar;

	//Forward declare the node
	struct _Node;

	//Our map
	typedef std::map <SearchChar,_Node*> SearchMap;

	//Our node
	typedef struct _Node
	{
		SearchChar		aChar;	//Our character
		int             bFinal; //Do we have a word here
		SearchMap		aMap;	//Our next nodes
		_Node*			pFailureNode;	//Where we go incase of failure
		unsigned short	usDepth;	//Depth of this level
	} Node;
private:
	//Add a string to a node
	void AddString(const SearchString& rString,
				   Node* pNode);

	//Search for a non final string (this is to build the index)
	//If not found then it will get the root node
	const Node* SearchNode(const SearchString& rString,
						   const Node* pNode)const;
	Node* SearchNode(const SearchString& rString,
					 Node* pNode);

	//Build the node index
	void BuildIndex(const SearchString& rString,
					Node* pNode);

	//Delete a node
	void DeleteNode(Node* pNode)const;

	//Clone a node
	Node* CloneNode(const Node* pNode)const;

	//Clone the entire trie
	void CloneTrie(CSuffixTrie& rTarget)const;

	//Insert a string into a vector
	void BuildStrings(StringsVector& rVector,
				      const SearchString& rString,
					  const Node* pNode)const;

	//Our root node
	Node m_aRoot;
};
