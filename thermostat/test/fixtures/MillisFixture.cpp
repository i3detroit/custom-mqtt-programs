#include "MillisFixture.h"

uint32_t SECRETMILLIS = 0;
uint32_t millis()
{
	return SECRETMILLIS;
}
