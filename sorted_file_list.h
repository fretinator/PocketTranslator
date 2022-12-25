#ifndef SORTED_FILE_LIST_H
#define SORTED_FILE_LIST_H

#include <Arduino.h>

class SortedFileList {
public:
  static const int max_file_name = 25;
	void rewind();
	char* next();
	void add(char* fileName);
	int getFileCount();

	SortedFileList();

  void dump(Stream& serialPort);

private:
// Our implementation is single-linked list
	struct FileNode {
		char fileName[max_file_name + 1];
		FileNode* nextNode;
	};

	FileNode* root = NULL; 
	FileNode* currentNode;
};


#endif