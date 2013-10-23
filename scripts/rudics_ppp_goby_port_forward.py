#!/usr/bin/python

# Copyright 2013
# Toby Schneider
# Licensed under the GNU Public License v3 (or later) 
#
# Handles incoming Iridium RUDICS traffic and port forwards as appropriate
# for PPP or Goby data

import asyncore
import socket

# this script listens (binds) on this port
rudics_address = '0.0.0.0'
rudics_port = 40000

# connections that send "goby" as the first four characters gets
# forwarded to a server listening on this server at this port
goby_server = "127.0.0.1"
goby_port = 40001

# connections that send "~" as the first character gets
# forwarded to a server listening on localhost at this port
ppp_server = "192.168.1.111"
ppp_port = 40002

class ConditionalForwardClient(asyncore.dispatcher_with_send):

    def __init__(self, server, host, port):
        asyncore.dispatcher_with_send.__init__(self)
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connect( (host, port) )
        self.server = server
           
    def handle_read(self):
        data = self.recv(8192)
        if data:
           self.server.send(data)


class ConditionalForwardHandler(asyncore.dispatcher_with_send):


    def __init__(self, sock, addr):
        asyncore.dispatcher_with_send.__init__(self, sock)
        self.identified_protocol = False
        self.client = None
        self.addr = addr

    def handle_read(self):
        data = self.recv(8192)
        if data:
            if not self.identified_protocol:
                print 'First message from this connection: %s bytes' % len(data)
                self.identified_protocol = True
                if data[0] == '~':
                    print "This is a PPP connection"
                    self.client = ConditionalForwardClient(self, ppp_server, ppp_port)
                elif len(data) >= 4 and data[0:4] == "goby":
                    print "This is a Goby connection"
                    self.client = ConditionalForwardClient(self, goby_server, goby_port)
                else:
                    print "This is a unknown connection"
                    self.close()
                    return
            self.client.send(data)            

    def handle_close(self):
        print 'Connection closed from %s' % repr(self.addr)
        self.close()


class ConditionalForwardServer(asyncore.dispatcher):

    def __init__(self, host, port):
        asyncore.dispatcher.__init__(self)
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.set_reuse_addr()
        self.bind((host, port))
        self.listen(5)

    def handle_accept(self):
        pair = self.accept()
        if pair is not None:
            sock, addr = pair
            print 'Incoming connection from %s' % repr(addr)
            handler = ConditionalForwardHandler(sock, addr)
            

server = ConditionalForwardServer(rudics_address, rudics_port)
asyncore.loop()
