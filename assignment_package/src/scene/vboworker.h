#ifndef VBOWORKER_H
#define VBOWORKER_H

#include <QRunnable>
#include <QMutex>
#include "terrain.h"

class VBOWorker : public QRunnable
{
private:
    Terrain* terrain;
    Chunk* chunk;
public:
    VBOWorker(Chunk* cp, Terrain* t);
    void run() override;
};

#endif // VBOWORKER_H
