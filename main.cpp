#include <iostream>

#include "src/utils/spsc_queue_v1.h"

int main()
{
    utils::SpscQueueV1<int> qv1(1000);
    return 0;
}
