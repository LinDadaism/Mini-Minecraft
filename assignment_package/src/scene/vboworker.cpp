#include "vboworker.h"

VBOWorker::VBOWorker(Chunk* cp, Terrain* t):
    terrain(t), chunk(cp)
{}

void VBOWorker::run()
{
    terrain->VBOWork(chunk);
}
