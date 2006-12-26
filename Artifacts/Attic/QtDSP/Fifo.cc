//	Fifo.cc:	std::vector FIFO implementation modeled on project sdr 
#include "Fifo.h"


/////////////////////////////////////////////////////////////////////////
Fifo::Fifo(int chunkSize):
  _chunkSize(chunkSize)
{
}

/////////////////////////////////////////////////////////////////////////
Fifo::~Fifo()
{
}

/////////////////////////////////////////////////////////////////////////
int
Fifo::size()
{
  return _deque.size();
}

/////////////////////////////////////////////////////////////////////////
int
Fifo::fpsize()
{
  return _fpdeque.size();
}

/////////////////////////////////////////////////////////////////////////
void
Fifo::push_back(short int* data)
{
  std::vector<short int> v;
  _deque.push_back(v);
  std::vector<short int>& back = _deque.back();
  back.resize(_chunkSize);
  for (int i = 0; i < _chunkSize; i++)
    back[i] = data[i];

}

void
Fifo::push_back(float * data)
{
  std::vector<float> v;
  _fpdeque.push_back(v);
  std::vector<float>& back = _fpdeque.back();
  back.resize(_chunkSize);
  for (int i = 0; i < _chunkSize; i++)
    back[i] = data[i];

}

/////////////////////////////////////////////////////////////////////////
std::vector<short int>&
Fifo::push_back()
{
  std::vector<short int> v;
  _deque.push_back(v);
  std::vector<short int>& back = _deque.back();
  back.resize(_chunkSize);
  return _deque.back();
}

/////////////////////////////////////////////////////////////////////////
std::vector<short int>&
Fifo::front()
{
  return _deque.front();
}

/////////////////////////////////////////////////////////////////////////
std::vector<float>&
Fifo::fpfront()
{
  return _fpdeque.front();
}

/////////////////////////////////////////////////////////////////////////
void
Fifo::pop_front()
{
  if (_deque.size() > 0)
    _deque.pop_front();
}

/////////////////////////////////////////////////////////////////////////
void
Fifo::pop_fpfront()
{
  if (_fpdeque.size() > 0)
    _fpdeque.pop_front();
}

/////////////////////////////////////////////////////////////////////////
void
Fifo::pop_back()
{
  if (_deque.size() > 0)
    _deque.pop_back();
}
