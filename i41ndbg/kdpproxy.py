# KDP uses UDP packets, so lldb client expects to send and receive UDP packets
# this works fine over wifi but that can be annoying depending on your network setup/speed
#
# the system usbmuxd daemon is capable of setting up TCP tunnels to the iDevice over
# the usb cable, but not UDP. (well, I haven't looked at usbmuxd but I can't find any
# writeups which make it sound like this is possible.)
#
# this is a simple script to wrap each UDP packet sent by lldb client and send it
# over a TCP stream

import sys
import socket
import select
import struct
import binascii

local_listen_port = 41139
remote_target_port = 41414

KDP_MAX_REQUEST_SIZE = 1472


def usage():
  print './kdpproxy remote.ip.addr'

# recv in a loop until length bytes have been read
def recv_buffer(sock, length):
  buf = ''
  while length > 0:
    try:
      received = sock.recv(length)
      buf += received
      length -= len(received)
    except:
      pass
  return buf

if len(sys.argv) < 2:
  usage()
  quit()

remote_host = sys.argv[1]

print 'remote host: %s' % remote_host

# listen for UDP messages on localhost:local_listen_port
client_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
client_sock.bind(('localhost', local_listen_port))
#client_sock.listen(1)

# connect to the KDP server running in TCP mode on the iDevice:
device_connection = socket.create_connection((remote_host, remote_target_port))
device_connection.setblocking(0)

# in a loop wait for a message on either the client socket or the device socket
inputs = [client_sock, device_connection]
client_host = None

while True:
  readable, writeable, exceptional = select.select(inputs, [], inputs)
  for s in exceptional:
    # handle error
    pass
  for s in readable:
    if s is client_sock:
      # there is a message from the lldb client to the proxy waiting, receive it:
      (buf, (client_host, _)) = s.recvfrom(KDP_MAX_REQUEST_SIZE)
      print client_host

      print 'received %d bytes message from udp socket' % len(buf)
      print binascii.hexlify(buf)

      # wrap it and send it over usbmuxd
      # wire format is a 4 byte LE length followed by the data:
      lenstr = struct.pack('<i', len(buf))
      device_connection.sendall(lenstr)
      device_connection.sendall(buf)

    if s is device_connection:
      # a message back from the device to the lldb client
      # format is: <port> <len> <data> where port and len are 4 byte LE
      #client_port_str = device_connection.recv(4, socket.MSG_WAITALL)
      client_port_str = recv_buffer(device_connection, 4)
      print 'len(client_port_str) = %d' % (len(client_port_str))
      client_port = struct.unpack("<i", client_port_str)[0]
     
      print 'client port: %d' % (client_port)
      
      #payload_len_str = device_connection.recv(4, socket.MSG_WAITALL)
      payload_len_str = recv_buffer(device_connection, 4)
      payload_len = struct.unpack("<i", payload_len_str)[0]

      print 'payload len: %d' % (payload_len)

      #payload = device_connection.recv(payload_len, socket.MSG_WAITALL)
      payload = recv_buffer(device_connection, payload_len)

      # send it to the lldb client as a UDP packet, using the correct target port
      if client_host is None:
        print 'message from the server before having receieved anything from the client'
        quit()

      client_sock.sendto(payload, (client_host, client_port))
