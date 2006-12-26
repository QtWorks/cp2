#ifndef FIFO_H	//	std::vector FIFO implementation modeled on project sdr 
#define FIFO_H

#include <deque>
#include <vector>

class Fifo {

 public:
  Fifo(int chunkSize);
  virtual ~Fifo();
  void push_back(short int* data);
  void push_back(float * data);
  std::vector<short int>& push_back();
  void pop_front();
  void pop_fpfront();
  void pop_back();
  std::vector<short int>& front();
  std::vector<float>& fpfront();
  int size();
  int fpsize();

  int _chunkSize;	//	make public so may access once data is received

 protected:
  std::deque<std::vector<short int> > _deque;
  std::deque<std::vector<float> > _fpdeque;
//  int _chunkSize;
};

#endif
