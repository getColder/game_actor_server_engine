#include "base_buffer.h"
#include <iostream>
#include <cstring>

unsigned Buffer::GetEmptySize()
{
	return _bufferSize - _endIndex;
}

/* 写数据缓冲区时判断是否容量够，不够需要对环形缓冲区进行 */
void Buffer::ReAllocBuffer(const unsigned int dataLength)
{
	/* 如果缓冲区超过最大缓冲值，可能有异常，直接关闭socket  */ 
	if (_bufferSize >= MAX_SIZE) {
		std::cout << "Buffer::Realloc except!! " << std::endl;
	}
	char* tempBuffer = new char[_bufferSize + ADDITIONAL_SIZE];
	unsigned int _newEndIndex;
	if (_beginIndex < _endIndex)
	{
		/* 首尾拷贝 */
		::memcpy(tempBuffer, _buffer + _beginIndex, _endIndex - _beginIndex);
		_newEndIndex = _endIndex - _beginIndex;
	}
	else
	{
		/* 环形尾首拷贝 */
		//....判空
		if (_beginIndex == _endIndex && dataLength <= 0)
		{	
			_newEndIndex = 0;   //头指针与尾指针重合，且有效数据为0 ，判断缓冲区为空。重置尾指针
		}
		else 
		{
			// 1.先COPY尾部
			::memcpy(tempBuffer, _buffer + _beginIndex, _bufferSize - _beginIndex);
			_newEndIndex = _bufferSize - _beginIndex;	//copy1

			// 2.再COPY头部
			if (_endIndex > 0)
			{
				::memcpy(tempBuffer + _newEndIndex, _buffer, _endIndex);
				_newEndIndex += _endIndex;		//copy1 + copy2
			}
		}
	}

	// 修改数据
	_bufferSize += ADDITIONAL_SIZE;

	delete[] _buffer;
	_buffer = tempBuffer;

	_beginIndex = 0; //重置头指针
	_endIndex = _newEndIndex;

	//std::cout << "Buffer::Realloc. _bufferSize:" << _bufferSize << std::endl;
}
