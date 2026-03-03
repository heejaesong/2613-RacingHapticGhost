import asyncio
import moteus

async def main():
    transport = moteus.Fdcanusb(path="COM7")
    c = moteus.Controller(id=1, transport=transport)

    while True:
        state = await c.query()
        pos = state.values[moteus.Register.POSITION]
        print(f"pos_rev={pos:.6f}")
        await asyncio.sleep(0.01)

asyncio.run(main())
