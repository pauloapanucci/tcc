from SimpleWebSocketServer import SimpleWebSocketServer, WebSocket
from websocket import create_connection
import json
import threading

current_shadow = ""


class SimpleEcho(WebSocket):

    def handleMessage(self):
        global current_shadow
        # echo message back to client
        # print self.data 
        if(self.data == "\"appshadow\""):
            # data = {}
            # test = 'testando123'
            # data['barcode'] = "teste" 
            # json_data = json.dumps(data)
            print "received appshadow request from web: "
            print "SHADDDDDOW " + current_shadow
            # if len(current_shadow) != 0:
            # shadowbckp = copy.deepcopy(current_shadow)
            print 'SHADOW FOUNDED: ', current_shadow
            self.sendMessage(str(current_shadow))
            # current_shadow = ''
            # else:
            #     print 'no shadow founded'
            #     self.sendMessage("no shadow founded")
        else:
            
            # print "received shadow from python: ", self.data
            # if len(self.data) == 12:
            current_shadow = self.data
            self.sendMessage("Shadow Successful received")
            # else: 
                # self.sendMessage("Seems to be a problem on the barcode received")
       
    def handleConnected(self):
        print(self.address, 'connected')

    def handleClose(self):
        print(self.address, 'closed')

server = SimpleWebSocketServer('', 5000, SimpleEcho)
server.serveforever()
