#ifndef MULTICAST_TEST_SUB_H
#define MULTICAST_TEST_SUB_H

extern "C" int mySubDebug;

extern "C"
{
typedef long (*processMethod)(subRecord *precord);

static long mySubInit(subRecord *precord,processMethod process);
static long mySubProcess(subRecord *precord);
}

#endif
