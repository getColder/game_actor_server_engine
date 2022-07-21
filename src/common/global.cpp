#include "global.h"

uint64 Global::GenerateSN( ) {
	std::lock_guard<std::mutex> guard( _mtx );
	uint64 ret = ( TimeTick << 32 ) + ( _serverId << 16 ) + _snTicket;
	_snTicket += 1;		//加锁，使SN码全局唯一
	return ret;
}
