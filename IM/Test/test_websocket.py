import asyncio
import websockets

async def test_connection():
    uri = "ws://localhost:8080/ws"
    async with websockets.connect(uri) as websocket:
        print("Connected to WebSocket server")
        await websocket.send("Hello, server!")
        response = await websocket.recv()
        print(f"Received from server: {response}")

asyncio.get_event_loop().run_until_complete(test_connection())