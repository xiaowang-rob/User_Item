#include "enc_spi_engine.h"

bool enc_engine_run(tEncSpiEngine *e, const tEncXferSeg *segs, uint8_t n)
{
    if (!e || !e->bus || !segs || n == 0U)
        return false;

    const tSpiBusIf *bus = e->bus;

    bus->cs(bus->ctx, true); // 选中芯片
    for (uint8_t i = 0U; i < n; i++)
    {
        if (!bus->xfer(bus->ctx, segs[i].tx, segs[i].rx, segs[i].len))
        {
            bus->cs(bus->ctx, false); // 失败立即释放总线
            return false;
        }
    }
    bus->cs(bus->ctx, false); // 完成，释放总线
    return true;
}

void enc_engine_abort(tEncSpiEngine *e)
{
    if (e && e->bus)
        e->bus->cs(e->bus->ctx, false);
}
