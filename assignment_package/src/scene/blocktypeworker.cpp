#include "blocktypeworker.h"

BlockTypeWorker::BlockTypeWorker(int xi, int zi, Terrain* t)
    : terrain(t), x(xi), z(zi)
{}


void BlockTypeWorker::run()
{
    terrain->BlockTypeWork(x, z);
}
