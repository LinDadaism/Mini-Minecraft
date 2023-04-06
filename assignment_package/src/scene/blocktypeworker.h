#ifndef BLOCKTYPEWORKER_H
#define BLOCKTYPEWORKER_H

#include <QRunnable>
#include <QMutex>
#include "terrain.h"

class BlockTypeWorker : public QRunnable
{
private:
    Terrain* terrain;
    int x;
    int z;
public:
    BlockTypeWorker(int xi, int zi, Terrain* t);
    void run() override;
};

#endif // BLOCKTYPEWORKER_H
