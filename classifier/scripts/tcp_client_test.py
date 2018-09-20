#!/usr/bin/env python3

import socket
import struct
import math

"""
This code is suposed to work after Simulink Denso code is serving.
It sends angle in radians to the simulation. This code is only a test.
Present functions will be implemented in RobotArm module.
"""

HOST = 'localhost'  # The server's hostname or IP address
PORT = 17001        # The port used by the server
s = socket.socket() # Instatiate object of socket

""" Hard-coded angles for testing """
angles = [math.pi/2.0, 0, -math.pi/3.0, math.pi/3.0, 0, 0]
"""
Struct each angle in C like format.

First characer "<" indicates byte order (little-endian in this case)
"6d" indicates 6 values of type double.
*angles only retrieve eache element from angles. Same as angles[0], angles[1]...
"""
data = struct.pack(b'<6d', *angles)

"""
Send data and close socket connection
"""
try:
    s.connect((HOST, PORT)) 
    s.send(data)
except Exception as e: 
    print("Something's wrong with %s:%d. Exception is %s" % (HOST, PORT, e))
finally:
    """
    If we close this socket, Server socket is also closed...
    That's why next line is commented.
    """
    # s.close()
    print("Sent!")

print('Finished program!')