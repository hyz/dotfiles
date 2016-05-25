#!/usr/bin/python3
import asyncio
 
async def start_server():
    await asyncio.start_server(client_connected_handler, '127.0.0.1', 2222)
 
async def client_connected_handler(client_reader, client_writer):
    peer = client_writer.get_extra_info('peername')
    print('Connected..%s:%s' % (peer[0], peer[1]))
    while True:
        data = await client_reader.read(1024)
        if not data:
            print('Disconnected..%s:%s\n' % (peer[0], peer[1]))
            break
        print(data.decode(), end='')
        client_writer.write(data)
 
loop = asyncio.get_event_loop()
server = loop.run_until_complete(start_server())
try:
    loop.run_forever()
except KeyboardInterrupt:
    pass
 
server.close()
loop.run_until_complete(server.wait_closed())
loop.close()

