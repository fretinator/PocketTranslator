#include "sorted_file_list.h"

void SortedFileList::rewind() {
  currentNode = root;
}

char* SortedFileList::next() {
  char* ret = NULL;

  if(NULL == currentNode) {
    currentNode = root;
  }

  if(currentNode != NULL) {
    ret = currentNode->fileName;
    currentNode = currentNode->nextNode;
  }
  
  return ret;
}

void SortedFileList::add(char* fileName) {
  FileNode* newNode = new FileNode();
  strncpy(newNode->fileName, fileName, max_file_name);
  newNode->nextNode = NULL;

  if(NULL == root) {
    root = newNode;
    currentNode = root;
    
  } else {
    if(strncmp(root->fileName, fileName, max_file_name) > 0) {
      newNode->nextNode = root;
      if(currentNode == root) {
        currentNode = newNode;
      }
      root = newNode;
    } else {
      // Needed two pointers since we have no prev.
      FileNode* thisNode = root;
      FileNode* nextNode = root->nextNode;


      while(NULL != nextNode && strncmp(nextNode->fileName, fileName, max_file_name) <= 0) {
        thisNode = nextNode;
        nextNode = nextNode->nextNode;
      }

      thisNode->nextNode = newNode; 
      newNode->nextNode = nextNode;
    }
  }
}

int SortedFileList::getFileCount() {
  int count = 0;

  for(FileNode* node = root; NULL != node; node = node->nextNode) {
    count++;
  }

  return count;
}

SortedFileList::SortedFileList() {
  root = NULL;
  currentNode = root;
}

void SortedFileList::dump(Stream& serialPort) {
  FileNode* node = root;

  while(NULL != node) {
    serialPort.print("File: ");
    serialPort.println(node->fileName);
    node = node->nextNode;
  }
}