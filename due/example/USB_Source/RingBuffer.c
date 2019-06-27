#include "RingBuffer.h"
#include <string.h>

struct RingBuffer _Rbuf;

void RingBuffer( void )
{
    memset( (void *)_Rbuf._aucBuffer, 0, SERIAL_BUFFER_SIZE ) ;
    _Rbuf._iHead=0 ;
    _Rbuf._iTail=0 ;
}

void store_char( uint8_t c )
{
  int i = (uint32_t)(_Rbuf._iHead + 1) % SERIAL_BUFFER_SIZE ;

  // if we should be storing the received character into the location
  // just before the tail (meaning that the head would advance to the
  // current location of the tail), we're about to overflow the buffer
  // and so we don't write the character or advance the head.
  if ( i != _Rbuf._iTail )
  {
	  _Rbuf._aucBuffer[_Rbuf._iHead] = c ;
	  _Rbuf._iHead = i ;
  }
}
